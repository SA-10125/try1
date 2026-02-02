const dotenv=require("dotenv") 
dotenv.config();//this lets us access our environment variables in the .env

const {runShatterlockEncrypt,readShatterlockDecrypt}=require('./shatterhandle.js') //functions for shatterlock for local filesystem

const connectDB=require("./config/db.js");
const ConnectedNode=require("./models/connectedNodes.js")
const NodeAndName=require("./models/nodesAndNames.js")

//create a http server
const http=require('http') //builtin library in node
server=http.createServer()

const socketIO=require('socket.io'); //importing socketio
const { disconnect } = require("cluster");
//socketIO is a function that will take our http server and upgrade it to a websockets server
const io=socketIO(server,{cors:{origin:'*'}}) //allowing access from anywhere (cors)
//it is an event based object so you listen for events and when those events happen you do things

io.on('connection',async (socket)=>{
    console.log("User connected: "+socket.id);
    //in database create that client/socket and set alive to true
    const node = new ConnectedNode({id:socket.id, alive:true, socket:socket});
    const savedNode = await node.save()

    // socket.emit('message',JSON.stringify({from:'Server', text:'Welcome',createdAt:Date.now()}))
    // socket.on('message',(msg)=>{
    //     console.log('msg recieved: '+msg+' from '+socket.id)
    //     io.emit('allAck','Hi all, server has recieved a message') 
    //     socket.emit('ack',`acknowledged: ${msg}`)
    // })
    // socket.on('ack',(msg)=>{console.log(`acknowledgement recieved: `,msg);})

    socket.on('init',async (msg)=>{
        msg=JSON.parse(msg)
        //in database set alive to true for that client/socket and fill in other details like link/adress
        const nodes=await ConnectedNode.find({id:socket.id})
        const node=nodes[0]
        node.adress=msg.adress;
        node.alive=true;
        await node.save()
        socket.emit('ack',`init message ${msg} recieved`)
        socket.emit('savedack',`init message ${msg} saved`)
        io.emit('announcement',`${socket.id} has connected`)
    })

    socket.on('encryptRequest',async (msg)=>{
        const nodes=await ConnectedNode.find({alive:true}).sort({createdAt:-1})
        socket.emit('availableNodesForEncrypt',JSON.stringify(nodes))
    })

    socket.on('nodesIWantEncryptWith',async (msg)=>{
        msg=JSON.parse(msg);
        nodes=msg.nodes
        confirmedNodes=[]
        myname=msg.myname
        //nodes contains all nodes that are to be sharing the secret.
        //might be outdated tho, get all them again from the db
        for(let i in nodes){const node=await ConnectedNode.find({id:i.id}); if (node.alive){confirmedNodes.push(node)};}
        //myname contains name of 'paragrapgh' to be distributed and encrypted.
        //now i will save the {myname:nodes} in the db so that i can refer to it throughout my encryption and decryption processes.
        //TODO: think if this compromises security.
        const nodeNname=new NodeAndName({name:myname,nodes:confirmedNodes})
        await nodeNname.save()
        //I WAS HERE (i guess now we send a response saying acknowledged and stored, all nodes you requested are available or not then the client sends the sentences)
    })

    socket.on('disconnect',async ()=>{
        console.log("User disconnected.");
        //in database set alive to false for that client/socket.
        const nodes=await ConnectedNode.find({id:socket.id})
        const node=nodes[0]
        node.alive=false;
        await node.save()
        io.emit('announcement',`${socket.id} has disconnected`)
    });

})

connectDB().then(()=>{
    server.listen(8080,console.log("WebSockets server started listening on localhost:8080"))
})


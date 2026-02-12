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
const { json } = require("stream/consumers");
//socketIO is a function that will take our http server and upgrade it to a websockets server
const io=socketIO(server,{cors:{origin:'*'}}) //allowing access from anywhere (cors)
//it is an event based object so you listen for events and when those events happen you do things

io.on('connection',async (socket)=>{
    console.log("User connected: "+socket.id);
    //in database create that client/socket and set alive to true
    const node = new ConnectedNode({id:socket.id, alive:true});
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
        const node = await ConnectedNode.findOne({id:socket.id})
        if (node) {
            node.adress=msg.adress;
            node.alive=true;
            await node.save()
            socket.emit('ack',`init message ${msg} recieved`)
            socket.emit('savedack',`init message ${msg} saved`)
            io.emit('announcement',`${socket.id} has connected`)
        }
    })

    socket.on('encryptRequest',async (msg)=>{
        const nodes=await ConnectedNode.find({alive:true}).sort({createdAt:-1})
        socket.emit('availableNodesForEncrypt',JSON.stringify(nodes))
    })

    socket.on('nodesIWantEncryptWith',async (msg)=>{
        msg=JSON.parse(msg);
        const nodes=msg.nodes
        const confirmedNodes=[]
        const myname=msg.name
        //nodes contains all nodes that are to be sharing the secret.
        //might be outdated tho, get all them again from the db
        for(let i of nodes){
            const node = await ConnectedNode.findOne({id:i.id});
            if (node && node.alive){// maybe here we should send a packet and get an ack confirming that its alive.
            confirmedNodes.push(node.id)};
        }
        //myname contains name of 'paragrapgh' to be distributed and encrypted.
        //now i will save the {myname:nodes} in the db so that i can refer to it throughout my encryption and decryption processes.
        //TODO: think if this compromises security.
        const nodeNname=new NodeAndName({name:myname,nodes:confirmedNodes})
        await nodeNname.save()
        //I WAS HERE (i guess now we send a response saying acknowledged and stored, all nodes you requested are available or not then the client sends the sentences)
        socket.emit('ackavailability',JSON.stringify({"msg":`${confirmedNodes.length} out of ${nodes.length} available`,"confirmedNodes":confirmedNodes,"asked":nodes,"nodeNname":nodeNname}));
    })

    socket.on('message',async (msg)=>{
        msg=JSON.parse(msg);
        const nodeNname = await NodeAndName.findOne({"name":msg.Name});
        if (!nodeNname || !nodeNname.nodes || nodeNname.nodes.length === 0) {
            console.log('No nodes found for name: ' + msg.Name);
            return;
        }
        const randomclientid = nodeNname.nodes[Math.floor(Math.random() * nodeNname.nodes.length)];
        contents=JSON.stringify({"from":msg.From,"contents":msg.Contents})
        //add junk ig, just demo of message sharing for now.
        //going with the most basic cipher i can think of for demo purposes
        let encryptedContents="";
        for(let i=0;i<contents.length;i++){encryptedContents+=String(contents.charCodeAt(i)+3)}
        //format of msg: |ED|To|OG|Name|From|Contents|
        const randomclient = await ConnectedNode.findOne({id:randomclientid});
        if(randomclient && randomclient.alive){ //send to that one client via io.to(socketid)
            io.to(randomclient.id).emit('message',JSON.stringify({"ED":"E","To":randomclient.id,"Name":msg.Name,"From":"server","Contents":encryptedContents}))
        }
    })

    socket.on('disconnect',async ()=>{
        console.log("User disconnected.");
        //in database set alive to false for that client/socket.
        const node = await ConnectedNode.findOne({id:socket.id})
        if (node) {
            node.alive=false;
            await node.save()
            io.emit('announcement',`${socket.id} has disconnected`)
        }
    });

})

connectDB().then(()=>{
    server.listen(8080,console.log("WebSockets server started listening on localhost:8080"))
})

//every once in a while, poll all active nodes to see if theyre still alive and update the db accordingly.
//for non alive nodes, change alive status to false and remove that from nodesNnames
//we also need a shutdown function for the server where it can clear the databases and then switch off.
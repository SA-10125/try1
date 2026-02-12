const dotenv=require("dotenv") 
dotenv.config();//this lets us access our environment variables in the .env

const io = require('socket.io-client')
const webSocketServerLink=process.env.WEBSOCKETSERVERLINK

const socket=io(webSocketServerLink,{
    autoConnect: true, //connect immideatly
    reconnection: true, //retry on failure
    reconnectionDelay: 1000, //wait time
    reconnectionAttempts: 5 //retry limit
}) //persistent connection to server

socket.on('connect',()=>{
    console.log('connected to server as: '+socket.id);
    sendInit()
})

socket.on('disconnect', () => {
    console.log('disconnected from server '+process.env.WEBSOCKETSERVERLINK)
})

// socket.on('message', (message) => { 
//     console.log('Received message:', message);
//     socket.emit('ack', `acknowledged ${message}`) 
// });

socket.on('ack',(ackmsg)=>{
        console.log('acknowledgement recieved: '+ackmsg)
})

socket.on('savedack',(ackmsg)=>{
        console.log('save acknowledgement recieved: '+ackmsg)
})

socket.on('announcement',(msg)=>{console.log("server has announced: ",msg);})

function sendInit(){
    socket.emit('init',JSON.stringify({"adress":`localhost:${process.env.PORT}`}));
    console.log("I will now send an encryption request in 5 secs.")
    setTimeout(sendEncryptRequest,5000);
}

function sendEncryptRequest(){
    socket.emit('encryptRequest','pls tell who all are ready to encrypt with me');
    console.log("encryption request sent.")
}

socket.on('availableNodesForEncrypt',(msg)=>{
    nodes=JSON.parse(msg)
    //now need some kind of UI to choose which nodes you want to share the secret with. (so will need names in the db and init msg also)
    //for now i will assume it is all of them.
    //need a UI for choosing name of file to be encrypted also.
    //  for now i will call it helloworldfile
    myname='helloworldfile' //+add some unique ID here such that no 2 file names are ever the same. V IMP
    socket.emit('nodesIWantEncryptWith',JSON.stringify({name:myname, nodes:nodes}))
})

socket.on('ackavailability',(msg)=>{
    msg=JSON.parse(msg);
    console.log(msg.msg);
    if(msg.confirmedNodes.length==msg.asked.length){ //TODO: look into, might not be right
        //all nodes we want are online, lets send the messages to the server which will distribute among nodes with bounces as needed.
        contents='helloworldhowareyouimgoodbroisallanyonesays'
        //add junk ig, just demo of message sharing for now.
        //going with the most basic cipher i can think of for demo purposes
        let encryptedContents="";
        for(let i=0;i<contents.length;i++){encryptedContents+=String(contents.charCodeAt(i)+3)}
        //format of msg: |ED|To|OG|Name|From|Contents|
        socket.emit('message',JSON.stringify({"ED":"E","To":"server","Name":'helloworldfile',"From":socket.id,"Contents":encryptedContents}))
    }
})

socket.on('message',(msg)=>{
    
})



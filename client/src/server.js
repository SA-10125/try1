const dotenv=require("dotenv") 
dotenv.config();//this lets us access our environment variables in the .env

const io = require('socket.io-client')
const webSocketServerLink=process.env.WEBSOCKETSERVERLINK

const path = require('path'); //might not need this after switching to shatterlock
const fs=require('fs'); //might not need this after switching to shatterlock


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
    console.log('[CLIENT] Received availableNodesForEncrypt:', msg);
    nodes=JSON.parse(msg)
    console.log('[CLIENT] Parsed nodes:', nodes);
    //now need some kind of UI to choose which nodes you want to share the secret with. (so will need names in the db and init msg also)
    //for now i will assume it is all of them.
    //need a UI for choosing name of file to be encrypted also.
    //  for now i will call it helloworldfile
    myname='helloworldfile' //+add some unique ID here such that no 2 file names are ever the same. V IMP
    console.log('[CLIENT] Sending nodesIWantEncryptWith with name:', myname);
    socket.emit('nodesIWantEncryptWith',JSON.stringify({name:myname, nodes:nodes}))
})

socket.on('ackavailability',(msg)=>{
    console.log('[CLIENT] Received ackavailability:', msg);
    msg=JSON.parse(msg);
    console.log('[CLIENT] Parsed ackavailability:', msg.msg);
    console.log('[CLIENT] Confirmed nodes:', msg.confirmedNodes.length, 'Asked nodes:', msg.asked.length);
    if(msg.confirmedNodes.length==msg.asked.length){ //TODO: look into, might not be right
        //all nodes we want are online, lets send the messages to the server which will distribute among nodes with bounces as needed.
        contents='helloworldhowareyouimgoodbroisallanyonesays'
        //add junk ig, just demo of message sharing for now.
        //going with the most basic cipher i can think of for demo purposes
        let encryptedContents="";
        for(let i=0;i<contents.length;i++){encryptedContents+=String(contents.charCodeAt(i)+3)}
        //format of msg: |ED|To|OG|Name|From|Contents|
        console.log('[CLIENT] Sending message event with Name: helloworldfile');
        socket.emit('message',JSON.stringify({"ED":"E","To":"server","OG":socket.id,"Name":'helloworldfile',"From":socket.id,"Contents":encryptedContents}))
    } else {
        console.log('[CLIENT] Not all nodes available, not sending message');
    }
})

// Create storagetemp directory at startup (temp, remove later when you switch to using shatterlock)
const storageDir = path.join(__dirname, '../storagetemp');
if (!fs.existsSync(storageDir)) {
    fs.mkdirSync(storageDir, { recursive: true });
    console.log('[CLIENT] Created storagetemp directory at:', storageDir);
}

function save(msg){
    console.log('[CLIENT] save() called with msg:', msg);
    //IMP TODO: for now. need a way to store multiple packets of the same Name with unique local names later.
    // Use relative path instead of absolute
    const filePath = path.join(storageDir, `${msg.Name}_${msg.From}_${Date.now()}.json`);
    console.log('[CLIENT] Saving to path:', filePath);
    fs.writeFile(filePath,JSON.stringify(msg),(err)=>{ 
        if(err){console.log(err);return false;}
        else{console.log("saved a "+msg.Name);return true;}
    }) 
}


function bounce(msg,socket){ //just gonna bounce the msg back to server after encrypting from my end with my key (later).
    console.log('[CLIENT] bounce() called, bouncing message back to server');
    //theoretically, we do need to confirm again saying are the nodes i want available? (for now imma assume they are)
    msg=JSON.parse(msg)
    console.log('[CLIENT] Parsed message in bounce():', msg);
    socket.emit('msgrecievedack', JSON.stringify({"ED":"E","To":"server","OG":msg.OG,"Name":msg.Name,"From":socket.id}))
    contents=JSON.stringify({"from":msg.From,"contents":msg.Contents})
    //add junk ig, just demo of message sharing for now.
    //going with the most basic cipher i can think of for demo purposes
    let encryptedContents="";
    for(let i=0;i<contents.length;i++){encryptedContents+=String(contents.charCodeAt(i)+3)} //for testing purposes, this is our encryption algorigthm.
    //format of msg: |ED|To|OG|Name|From|Contents|
    console.log('[CLIENT] Bouncing message back to server');
    socket.emit('message',JSON.stringify({"ED":"E","To":"server","OG":msg.OG,"Name":msg.Name,"From":socket.id,"Contents":encryptedContents}))
}

function keep(msg,socket){
    console.log('[CLIENT] keep() called');
    //im going to make this one save it to shatterlock soon but for now im just gonna save it in a .txt for proof of concept.
    msg=JSON.parse(msg)
    console.log('[CLIENT] Parsed message in keep():', msg);
    socket.emit('msgrecievedack', JSON.stringify({"ED":"E","To":"server","OG":msg.OG,"Name":msg.Name,"From":socket.id}))
    const saved=save(msg)
    if(saved){
        socket.emit('msgsavedack',JSON.stringify({"ED":"E","To":"server","OG":msg.OG,"Name":msg.Name,"From":socket.id}))
    }
}

socket.on('message',(msg)=>{
    console.log('[CLIENT] *** Received message event ***:', msg);
    //decide on a chance wether to save it or bounce it again to the server based on chance (and maybe a minimum quota).
    //if we save it, need to emit a savedack and recievedack
    //else need to emit a recievedack (maybe even a bouncedack?)
    const choices=[true,false,false,false] //weighted array (for now) (true=keep, false=bounce)
    const shouldKeep=choices[Math.floor(Math.random()*choices.length)]
    console.log('[CLIENT] Decision:', shouldKeep ? 'KEEP' : 'BOUNCE');
    if(shouldKeep){keep(msg,socket)}
    else{bounce(msg,socket)}
})


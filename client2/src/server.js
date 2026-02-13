const dotenv=require("dotenv") 
dotenv.config();//this lets us access our environment variables in the .env

const io = require('socket.io-client')
const webSocketServerLink=process.env.WEBSOCKETSERVERLINK

const path = require('path'); //might not need this after switching to shatterlock
const fs=require('fs'); //might not need this after switching to shatterlock

const express = require("express");
const app=express();

//middleware (to allow for post data to be passed via request.body)
app.use(express.json()); //this middleware will parse the json body and let us get access to the json body (req.body)
//middleware commes into play just before the response is sent back.

app.listen(process.env.PORT, () => {console.log("client started on port:"+process.env.PORT);});

global.Gcontents; //we will change to a more streamlined method soon i hope. for now the functions are not calling one after another, theyre relying on the server to send certain msgs which could be an issue with async and stuff, so look into streamlining the process more later.
global.Gname;

app.post('/api/encrypt',(req,res)=>{
    global.Gname=req.body.Gname;
    global.Gcontents=req.body.Gcontents;
    sendEncryptRequest();
    res.status(200).json({"message":"encryption request sent"});
})

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
    //+add some unique ID here such that no 2 file names are ever the same. V IMP
    socket.emit('nodesIWantEncryptWith',JSON.stringify({name:global.Gname, nodes:nodes}))
})
//when we add ui for choosing the nodes, we need to figure out a way to give all the nodes names instead of just the socket id's and having those stay constant with the adresses or smthn such that you can init with the socket id and that name since each time the socket id will be different, this will allow them to connect and disconnect and reconnect at a later date time and place.

//later we will want nothing to be stored local, they should all be indipendent functions working only based off of the messages that they get and no assumptions.
socket.on('ackavailability',(msg)=>{
    msg=JSON.parse(msg);
    if(msg.confirmedNodes.length==msg.asked.length){ //TODO: look into, might not be right
        //all nodes we want are online, lets send the messages to the server which will distribute among nodes with bounces as needed.
        //add junk ig, just demo of message sharing for now.
        //going with the most basic cipher i can think of for demo purposes
        let encryptedContents="";
        for(let i=0;i<global.Gcontents.length;i++){encryptedContents+=String.fromCharCode(global.Gcontents.charCodeAt(i)+parseInt(process.env.MYLOCALADD))} //local first encrypt
        //format of msg: |ED|To|OG|Name|From|Contents|
        global.Gcontents=null; //cleanup for safety purpose
        console.log('[CLIENT] Sending message with Name:'+global.Gname);
        socket.emit('message',JSON.stringify({"ED":"E","To":"server","OG":socket.id,"Name":global.Gname,"From":socket.id,"Contents":encryptedContents}))
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

function save(msg,socket){
    //IMP TODO: for now. need a way to store multiple packets of the same Name with unique local names later.
    // Use relative path instead of absolute
    const filePath = path.join(storageDir, `${msg.Name}_${msg.From}_${Date.now()}.json`);
    fs.writeFile(filePath,JSON.stringify(msg),(err)=>{ 
        if(err){
            console.log(err);
            bounce(msg,socket);
        }
        else{
            console.log("saved a "+msg.Name);
            socket.emit('msgsavedack',JSON.stringify({"ED":"E","To":"server","OG":msg.OG,"Name":msg.Name,"From":socket.id}))
        }
    }) 
}


function bounce(msg,socket){ //just gonna bounce the msg back to server after encrypting from my end with my key (later).
    //theoretically, we do need to confirm again saying are the nodes i want available? (for now imma assume they are)
    msg=JSON.parse(msg)
    socket.emit('msgrecievedack', JSON.stringify({"ED":"E","To":"server","OG":msg.OG,"Name":msg.Name,"From":socket.id}))
    let contents=JSON.stringify({"from":msg.From,"contents":msg.Contents})
    //add junk ig, just demo of message sharing for now.
    //going with the most basic cipher i can think of for demo purposes
    let encryptedContents="";
    for(let i=0;i<contents.length;i++){encryptedContents+=String.fromCharCode(contents.charCodeAt(i)+parseInt(process.env.MYADD))} //for testing purposes, this is our encryption algorigthm.
    //format of msg: |ED|To|OG|Name|From|Contents|
    contents=null; //cleanup for safety purposes
    socket.emit('message',JSON.stringify({"ED":"E","To":"server","OG":msg.OG,"Name":msg.Name,"From":socket.id,"Contents":encryptedContents}))
}

function keep(msg,socket){
    //im going to make this one save it to shatterlock soon but for now im just gonna save it in a .txt for proof of concept.
    msg=JSON.parse(msg)
    socket.emit('msgrecievedack', JSON.stringify({"ED":"E","To":"server","OG":msg.OG,"Name":msg.Name,"From":socket.id}))
    const saved=save(msg,socket)
}

socket.on('message',(msg)=>{
    //decide on a chance wether to save it or bounce it again to the server based on chance (and maybe a minimum quota).
    //if we save it, need to emit a savedack and recievedack
    //else need to emit a recievedack (maybe even a bouncedack?)
    const choices=[true,false,false,false] //weighted array (for now) (true=keep, false=bounce)
    const shouldKeep=choices[Math.floor(Math.random()*choices.length)]
    console.log('[CLIENT] Decision:', shouldKeep ? 'KEEP' : 'BOUNCE');
    if(shouldKeep){keep(msg,socket)}
    else{bounce(msg,socket)}
})


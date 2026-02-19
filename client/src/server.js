const dotenv=require("dotenv") 
dotenv.config();//this lets us access our environment variables in the .env

const io = require('socket.io-client')
const webSocketServerLink=process.env.WEBSOCKETSERVERLINK

const path = require('path'); //might not need this after switching to shatterlock
const fs=require('fs'); //might not need this after switching to shatterlock
const { readdir } = require("fs").promises;

const express = require("express");
const { json } = require("stream/consumers");
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

app.post('/api/decrypt',(req,res)=>{
    socket.emit('sendallof',JSON.stringify({"Name":req.body.Name,"From":socket.id,"To":"server","OG":socket.id}))
    res.status(200).json({"message":"decryption request sent"})
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
        let mycontents=JSON.stringify({"Contents":global.Gcontents})
        for(let i=0;i<mycontents.length;i++){encryptedContents+=String.fromCharCode(mycontents.charCodeAt(i)+parseInt(process.env.MYLOCALADD))} //local first encrypt
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
    const filePath = path.join(storageDir, `${msg.Name}_${msg.OG}_${Date.now()}.json`);
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
    socket.emit('msgrecievedack', JSON.stringify({"ED":"E","To":"server","OG":msg.OG,"Name":msg.Name,"From":socket.id}))
    let contents=JSON.stringify({"whoami":socket.id,"contents":msg.Contents})
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
    socket.emit('msgrecievedack', JSON.stringify({"ED":"E","To":"server","OG":msg.OG,"Name":msg.Name,"From":socket.id}))
    const saved=save(msg,socket)
}

socket.on('message',(msg)=>{
    msg=JSON.parse(msg)
    if(msg.ED=="E"){
        //decide on a chance wether to save it or bounce it again to the server based on chance (and maybe a minimum quota).
        //if we save it, need to emit a savedack and recievedack
        //else need to emit a recievedack (maybe even a bouncedack?)
        const choices=[true,false,false,false] //weighted array (for now) (true=keep, false=bounce)
        const shouldKeep=choices[Math.floor(Math.random()*choices.length)]
        console.log('[CLIENT] Decision:', shouldKeep ? 'KEEP' : 'BOUNCE');
        if(shouldKeep){keep(msg,socket)}
        else{bounce(msg,socket)}
    }
    else if(msg.ED=="D"){
        //decrypt one layer and see who its from and send to server with to=the client.
        //here contents is going to be an encryptedcontents containing from and contents
        try{
            let decryptedcontents="";
            for(let i=0;i<msg.Contents.length;i++){decryptedcontents+=String.fromCharCode(msg.Contents.charCodeAt(i)-parseInt(process.env.MYADD))} //for testing purposes, this is our decryption algorigthm.
            const parsedData=JSON.parse(decryptedcontents);
            const whoami=parsedData.whoami
            console.log(whoami); //for debugging
            const contents=parsedData.contents
            socket.emit('message',JSON.stringify({"ED":"D","To":"server","OG":msg.OG,"Name":msg.Name,"From":socket.id,"Contents":contents}))
        }
        catch(error){ //
            if(error.name=="SyntaxError" && msg.OG==socket.id){ //likely meaning the JSON couldnt be parsed meaning ours might be the last layer of local encryption
                try{
                    let decryptedcontents="";
                    for(let i=0;i<msg.Contents.length;i++){decryptedcontents+=String.fromCharCode(msg.Contents.charCodeAt(i)-parseInt(process.env.MYLOCALADD))} //local first encrypt
                    const contents=JSON.parse(decryptedcontents).Contents
                    console.log(contents);
                    // TODO: Need to implement way to send this back to the GET API request
                }
                catch(innerError){
                    console.log('[CLIENT] Decryption failed with both keys:', innerError.message);
                    console.log('[CLIENT] Decrypted data was:', decryptedcontents);
                }
            }
            else{console.log('[CLIENT] Error during decryption:', error)}
        }
    }
})

socket.on('allsend',async (msg)=>{
    msg=JSON.parse(msg);
    console.log("allsend request recieved"+msg)
    //find all files in your storagetemp that start with msg.name
    try {
       // readdir returns an array of file and directory names
       const files = await readdir(storageDir);
       for(const i of files){
        const name=i.split('_',2);
        if(name[0]==msg.name){
            //this file should be sent, so lets send.
            const filePath = path.join(storageDir, i);
            fs.readFile(filePath, 'utf8', (err,data)=>{
                if (err) {
                    console.error("Error reading file:", err);
                    return;
                }
                const savedMsg = JSON.parse(data);
                socket.emit('message',JSON.stringify({"ED":"D","To":"server","OG":name[1],"Name":name[0],"From":socket.id,"Contents":savedMsg.Contents}))
            })
        }
       }
    } catch (err) {
        console.error("Error reading directory:", err);
        return;
    }
})

// ============================================================================
// DECRYPTION IMPLEMENTATION - CURRENTLY MISSING
// ============================================================================
// CRITICAL: The decryption flow is not yet implemented. Here's what's needed:
// 
// 1. GET API endpoint: app.get('/api/decrypt') to trigger decryption request
// 2. Socket event 'decryptRequest' to tell server which file name to decrypt
// 3. Server broadcasts to all nodes to send saved packets with that name
// 4. Each node reads from storagetemp/ and emits messages with ED="D"
// 5. Messages flow: Node -> Server -> Previous Node -> ... -> Originator
// 6. Each stop decrypts one layer using their MYADD key
// 7. Originator decrypts final layer with MYLOCALADD and gets original content
// 8. Return decrypted content to GET API (suggestion: use Promise or global variable)
//
// LOGICAL ISSUES TO ADDRESS:
// - NodeAndName is deleted after first save, but needed for decryption routing
// - Need to track which nodes have copies (currently only one saves due to early delete)
// - Each bounce only knows immediate previous node, not full chain
// - Need mechanism to collect all saved packets (multiple nodes may have saved)
// - GET request needs async handling to wait for decryption completion
// ============================================================================

//now need to impliment a get api call where we return the decrypted contents.
//now in that, we need another socket event where we send a msg to server saying pls decrypt this msg
//now server sends a message to all nodes in the nodeAndName with that name saying ok guys start sending your msgs with this name.
//those should start sending the msgs and send an ack once all the msgs of that name in thier local storage has been sent out
//these will start msgs going out in ED=D sense. and keep going and get decrypted and eventually reach the source fully decrypted
//then somehow we will have to send back the reply to the get request with that fully decrypted contents
//now one way im thinking to do this is to to have the get api function keep a global variable in poll 
// and once the fully decrypted contents is gotten, it can update that variable and then the get api function realises it and sends the reply.
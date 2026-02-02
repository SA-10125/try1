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
}

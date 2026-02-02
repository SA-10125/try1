const mongoose=require('mongoose');

//step1: create a schema (schema meaning blueprint that defines how data is organized and its relations etc)
//step2: create a model based off the schema.

const connectedNodesSchema = new mongoose.Schema({
    //here we put our fields. (this whole thing in the argument is an object/objects.)
    id:{
        type:String,
    },
    adress:{
        type:String,
    },
    alive:{
        type:Boolean,
    },
    socket:{
        type: mongoose.Schema.Types.Mixed //this means anything goes
    }
},{timestamps:true} //this is the second object timestamps. by default, mongodb will give you createdAt and updatedAt fields.
)

const ConnectedNode=mongoose.model("ConnectedNode",connectedNodesSchema)
module.exports=ConnectedNode;
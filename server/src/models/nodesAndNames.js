const mongoose=require('mongoose');

//step1: create a schema (schema meaning blueprint that defines how data is organized and its relations etc)
//step2: create a model based off the schema.

const nodesAndNamesSchema=new mongoose.Schema({
    name:{
        type:String,
    },
    nodes:{
        type: mongoose.Schema.Types.Mixed //this means anything goes
    }
},{timestamps:true} //this is the second object timestamps. by default, mongodb will give you createdAt and updatedAt fields.
)

const NodeAndName=mongoose.model("NodeAndName",nodesAndNamesSchema)
module.exports=NodeAndName
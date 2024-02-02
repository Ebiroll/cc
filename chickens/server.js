

var save_response = {
    "status": "success"
//          "message"   : "Not implemented yet"
};

const { MongoClient } = require("mongodb");

const  uri  = "mongodb://localhost:27017";

const client = new MongoClient(uri);

// Database Name
const dbName = 'cc';



// var mc = require('mongodb').MongoClient;


// Returns all chickens positions
exports.serveChickens=function serveChickens( socket )
{
        var array = new Array();
        console.log("give me chickens");
        mc.connect(dbs, function(err, db) {
        if (err)
            return(array);
            //throw(err);
            db.collection("chickens", function(err, chickens) {
            console.log("serving chickens");

           
            chickens.find().sort({ _id : 1},function(err, cursor) {
                var j = 0;
                cursor.each(function(err, item) {
                    console.log("found",item);                        
                    if (item) {
                        //console.log(item);
                        if (item.data)
                        {
                            if (Number(item.data.active)===1)
                            {
                                socket.emit('load:chickens', item.data);
                                console.log("serve",item.data);
                                //item.data.selected=false;
                                array.push(item.data);
                            }
                            
                        }
                    }
                    else
                    {   
                        console.log("my array",array);
                        db.close();
                        return(array);
                    }
                });
            });
        });
    });
}

exports.chickensGet = function(request, response)
{
    console.log("chickensGet");


    async function main(request, response) {
        // Use connect method to connect to the server
        await client.connect();
        console.log('Connected successfully to get from server');
        const db = client.db(dbName);
        const collection = db.collection('chickens');
      
        // the following code examples can be pasted here...
        const findResult = await collection.find({}).toArray();
        console.log('Found documents =>', findResult);

        var result_data = {
            "status": "sucess",
            "total": findResult.length,
            "records": findResult
        };

        response.write(JSON.stringify(result_data));
        response.end();
      }

      main(request, response).catch(console.error);
};



exports.chickensPost = function(request, response)
{
    //console.log("post",req.);
    console.log("post chickens body", request.body);
    console.log("serverGet", request.body.cmd);
    response.type('json');

    response.writeHead(200, {
        'Content-type': 'application/json',
        'Access-Control-Allow-Origin': '*'
    });

    switch (request.body.cmd)
    {
        case 'get-records':
            {
                console.log("Hallelulja chickens");
                exports.chickensGet(request, response);
            }
            break;
        case 'save-record':
            {
                console.log("SAVE RECORD");
                async function save(request, response) {
                    // Use connect method to connect to the server
                    await client.connect();
                    console.log('Connected successfully to server');
                    console.log("save-record", request.body.record);
                    const db = client.db(dbName);
                    const collection = db.collection('chickens');
                  
                    // recid: request.body.record["_id"],
                    var doc = {
                        _id: Number(request.body.record["_id"]),
                        ...request.body.record
                    };
                
                    // the following code examples can be pasted here...
                    const insertResult = await collection.insertOne(doc);

                    response.write(JSON.stringify(save_response));
                    response.end("\r\n");
                  }
            
                  save(request, response).catch(console.error);
            
            }

            break;
        case 'delete-records':
            var delsel = request.body.selected;
            console.log("Delete RECORD ,", Number(delsel[0]));


            var ok_response = {
                "status": "sucess"
//                    "message"   : "Not implemented"
            };
            response.write(JSON.stringify(ok_response));
            console.log(delsel);
            response.end();
            break;
        default:
            {
                response.write(save_response);
                response.end();
            }
    }

};




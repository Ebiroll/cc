

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

exports.serveChickens = function serveChickens( socket )
{
    console.log("serveChickens");

    async function main(socket) {
        // Use connect method to connect to the server
        await client.connect();
        console.log('Connected successfully to get from server');
        const db = client.db(dbName);
        const collection = db.collection('chickens');
      
        // the following code examples can be pasted here...
        const findResult = await collection.find({}).toArray();
        console.log('Found documents =>', findResult);

        // Loop through the results and emit the data to the socket
        findResult.forEach(function(item) {
            console.log("found",item);                        
            if (item) {
                console.log(item);
                if (item)
                {
                    if (Number(item.active)===1)
                    {
                        socket.emit('load:chickens', item);
                        console.log("serve",item);
                    }
                    
                }
            }
        });


      }

      main(socket).catch(console.error);
};


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
    console.log("post chickens body", request);
    console.log("serverGet", request.query);
    response.type('json');

    response.writeHead(200, {
        'Content-type': 'application/json',
        'Access-Control-Allow-Origin': '*'
    });
    console.log(request.query.action)

    const requestObject = JSON.parse(request.query.request);

    switch (requestObject.action)
    {
        case 'get':
            {
                console.log("Hallelulja chickens");
                exports.chickensGet(request, response);
            }
            break;
        case 'save':
            {
                console.log("SAVE RECORD");
                async function save(request, response) {
                    // Use connect method to connect to the server
                    await client.connect();
                    console.log('Connected successfully to server');
                    console.log("save-record", requestObject.record);
                    const db = client.db(dbName);
                    const collection = db.collection('chickens');
                  
                    // recid: request.body.record["_id"],
                    //  _id: Number(requestObject.record["_id"]),
                    var doc = {                       
                        ...requestObject.record
                    };
                
                    // the following code examples can be pasted here...
                    const insertResult = await collection.insertOne(doc);
                    console.log('Inserted documents =>', insertResult);

                    response.write(JSON.stringify(save_response));
                    response.end("\r\n");
                  }
            
                  save(request, response).catch(console.error);
            
            }

        break;
        case 'delete':
                {
                        console.log("DELETE RECORDS");
                        async function deleteRecords(request, response) {
                            // Use connect method to connect to the server
                            await client.connect();
                            console.log('Connected successfully to server');
                            
                            var delsel = request.body.selected.map(Number); // Assuming this is an array of IDs to delete
                            console.log("Delete RECORDS:", delsel);
                
                            const db = client.db(dbName);
                            const collection = db.collection('chickens');
                
                            // Assuming '_id' is used to identify documents to be deleted
                            const deleteResult = await collection.deleteMany({
                                _id: { $in: delsel }
                            });
                
                            var ok_response = {
                                "status": "success",
                                "deletedCount": deleteResult.deletedCount // Number of documents deleted
                            };
                
                            response.write(JSON.stringify(ok_response));
                            console.log(`Documents deleted: ${deleteResult.deletedCount}`);
                            response.end("\r\n");
                        }
                
                        deleteRecords(request, response).catch(console.error);
                    }
                break;
                default:
            {
                response.write(save_response);
                response.end();
            }
    }

};




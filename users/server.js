
var save_response = {
    "status": "success"
//          "message"   : "Not implemented yet"
};

const { MongoClient } = require("mongodb");

const  uri  = "mongodb://localhost:27017";

const client = new MongoClient(uri);

// ACHTUNG Todo, remove
var mc = require('mongodb').MongoClient;

// Database Name
const dbName = 'cc';

exports.usersAdd = function( username) {
    async function main(username) {
        try {
            // Use connect method to connect to the server
            await client.connect();
            console.log('Connected successfully to the server');

            const db = client.db(dbName);
            const collection = db.collection('users');

            console.log("Add user:", username);

            // Use findOne to check if the user already exists
            const userExists = await collection.findOne({ name: username });

            if (!userExists) {
                var record = {
                    _id: username,
                    name: username,
                    points: 0
                };

                // Insert the new user since they don't exist
                await collection.insertOne(record);

                console.log("User added successfully:", username);
            } else {
                console.log("User already exists:", username);
            }
        } catch (err) {
            console.error("An error occurred:", err);
        }
    }

    main(username).catch(console.error);
};


exports.myUsersAdd= function(username) {
    async function main(username) {
        // Use connect method to connect to the server
        await client.connect();
        console.log('Connected successfully to get from server');
        const db = client.db(dbName);
        const collection = db.collection('users');

        console.log("Add user!!",username);
        const findResult = await collection.find({}).toArray();
        //console.log('Found documents =>', findResult);
        // Only add if not found
        var found=false;
        for (var ix=0;ix<findResult.length;ix++)
        {
            if (findResult[ix].name===username)
            {
                found=true;
            }
        }
        if (found===false)
        {                     
            var record = {
                _id: username,
                name: username,
                points: 0
            }
            var doc = {_id: username,
                    recid: findResult.length+1,
                    ...record
                };

            collection.save(doc, {w: 1}, function(err, docs) {
                if (err) {
                    console.log("failed save",username);
                }
            });
        }
    }

    main(username).catch(console.error);
};


exports.servePosition = function servePosition( socket )
{
    console.log("servePosition");

    async function main(socket) {
        // Use connect method to connect to the server
        await client.connect();
        console.log('Connected successfully to get positions from server');
        const db = client.db(dbName);
        const collection = db.collection('positions');

        console.log("serving positions");
      
        // the following code examples can be pasted here...
        const findResult = await collection.find({}).toArray();
        //console.log('Found documents =>', findResult);

        // Loop through the results and emit the data to the socket
        findResult.forEach(function(item) {
            console.log("found",item);                        
            if (item) {
                console.log(item);
                if (item)
                {
                    if (item.data && item.data.coords.length > 0) {
                        socket.emit('load:coords', item.data);
                        console.log("coords",item.data);
                    } else {
                        console.log("No documents found.");
                    }
                }                    
            }
        });


      }

      main(socket).catch(console.error);
};

updatePoints = function(thePos) {
    console.log("update points", thePos);
    
    async function main(thePos) {
        try {
            // Use connect method to connect to the server
            await client.connect();
            console.log('Connected successfully to update points *****');
            const db = client.db(dbName);
            const collection = db.collection('users');

            const filter = { _id: thePos.id }; // Filter to match the document with the given _id
            const update = { $inc: { points: 1 } }; // Increment the points by 1
            const options = { upsert: false }; // Don't create a new document if it doesn't exist

            // Attempt to update the document
            const updateResult = await collection.updateOne(filter, update, options);

            // Log the result of the update operation
            if (updateResult.matchedCount && updateResult.modifiedCount) {
                console.log(`Successfully updated the document with _id: ${thePos.id}.`);
            } else {
                console.log(`Document with _id: ${thePos.id} not found or not updated.`);
            }
        } catch (err) {
            console.error(`Error updating points: ${err}`);
        } finally {
            // Ensuring that the client will close when you finish/error
            await client.close();
        }
    }

    main(thePos).catch(console.error);
};



checkForChickens=function(thePos,socket)
{
    console.log("give me chickens", thePos);

    async function main(thePos,socket) {
        try {
            // Assuming client is connected or will handle reconnection if necessary
            const db = client.db(dbName);
            const chickens = db.collection("chickens");

            console.log("checking chickens");

            // Use find with sort directly and convert to array to handle asynchronously
            const chickenItems = await chickens.find().sort({_id: 1}).toArray();

            for (const item of chickenItems) {
                //console.log("found---", item);
                if (item && Number(item.active) === 1) {
                    if ((Math.abs(Number(thePos.coords[0].lat) - Number(item.lat)) < 0.01) &&
                        (Math.abs(Number(thePos.coords[0].lng) - Number(item.lng)) < 0.01)) {
                        socket.emit('found:chicken', item.smallimg);
                        console.log("found@@@@@@@@", item);
                        await updatePoints(thePos); // Assuming updatePoints is an async function

                        // Update the item's active status
                        item.active = "0";
                        await chickens.updateOne({_id: item._id}, {$set: {"active": "0"}});
                    }
                }
            }
        } catch (err) {
            console.error("An error occurred while checking for chickens:", err);
        }
    } 
    main(thePos,socket).catch(console.error);

};

exports.savePosition=function(thePos,socket)
{
    console.log("SAVE Positions");
    checkForChickens(thePos,socket)

    async function main(thePos,socket) {
        try {
            // Use connect method to connect to the server
            await client.connect();
            console.log('Connected successfully to get from server');
            const db = client.db(dbName);
            const collection = db.collection('positions');
        
            const findResult = await collection.find({}).toArray();
            //console.log('Found positions documents =>', findResult);

            // Prepare the document to be upserted
            var doc = {
                $set: {
                    recid: 0, // This might need to be dynamically set or updated in some way
                    data: thePos
                }
            };

            // Use updateOne with upsert option
            const updateResult = await collection.updateOne({_id: thePos.id}, doc, {upsert: true});
            console.log('Upserted document =>', updateResult);
        }  catch (err) {
            console.error("An error occurred:", err);
        }
    };

    main(thePos).catch(console.error);
}
//  finally {
            // Ensure that the client will close when you finish/error
//            await client.close();
//        }



exports.usersGet = function(request, response)
{
    console.log("usersGet");

    async function main(request, response) {
        // Use connect method to connect to the server
        await client.connect();
        console.log('Connected successfully to get from server');
        const db = client.db(dbName);
        const collection = db.collection('users');
      
        // the following code examples can be pasted here...
        const findResult = await collection.find({}).toArray();
        //console.log('Found documents =>', findResult);

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



exports.usersPost = function(request, response)
{
    //console.log("post",req.);
    console.log("body", request.body);
    console.log("serverGet", request.body.cmd);
    response.type('json');

    response.writeHead(200, {
        'Content-type': 'application/json',
        'Access-Control-Allow-Origin': '*'
    });

    var active_request = '';
    if (request.query && request.query.request) {
        active_request = request.query.request;
    }
    
    let requestObject;
    // Attempt to parse active_request if it's not an empty string
    if (active_request) {
        requestObject = JSON.parse(active_request);
    } else {
        // Handle the case where active_request is empty or undefined
        // For example, by setting requestObject to null, an empty object, or a default value
        requestObject = {}; // or null, or any other default value appropriate for your situation
    }


    switch (request.body.cmd)
    {
        case 'get-records':
            {
                console.log("Hallelulja users");
                exports.usersGet(request, response);
            }

            break;
        case 'save-record':
            {
                console.log("SAVE RECORD");
                async function save(request, response) {
                    // Use connect method to connect to the server
                    await client.connect();
                    console.log('Connected successfully to server');
                    console.log("save-record", requestObject.record);
                    const db = client.db(dbName);
                    const collection = db.collection('users');
                  
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
        case 'delete-records':
            {
                var delsel = request.body.selected;
                console.log("Delete RECORD ,", Number(delsel[0]));

                console.log("DELETE RECORDS");
                async function deleteRecords(request, response) {
                    // Use connect method to connect to the server
                    await client.connect();
                    console.log('Connected successfully to server');

                    var delsel = request.body.selected.map(Number); // Assuming this is an array of IDs to delete
                    console.log("Delete RECORDS:", delsel);

                    const db = client.db(dbName);
                    const collection = db.collection('users');

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



exports.positionPost = function(request, response)
{
    //console.log("post",req.);
    console.log("body", request.body);
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
                console.log("Hallelulja get");
                ///var second = JSON.stringify(fake); 
                //response.write(second);

                mc.connect(dbs, function(err, db) {
                    if (err)
                        throw(err);
                        db.collection("positions", function(err, collection) {
                        console.log("find!!");

                        var array = new Array();
                        var cursor = collection.find().sort({ "_id" : 1},function(err, cursor) {
                            var j = 0;
                            cursor.each(function(err, item) {
                                if (item) {
                                    //console.log(item);
                                    if (item)
                                    {
                                        console.log("users",item);
                                        item.data.selected=false;
                                        var Activ='0';
                                        if (item.active) 
                                        {
                                            Activ='1'
                                        }
                                        
                                        var flattened={
                                            _id :   item._id,
                                            active : Activ,
                                            recid : j+1,
                                            data :  item.data,
                                            lat : item.data.coords[0].lat,
                                            lng : item.data.coords[0].lng,
                                            acr : item.data.coords[0].acr
                                        };
                                        
                                        console.log(item.data.coords);
                                        
                                        array.push(flattened);
                                        array[j].recid = j+1;

                                        j++;

                                        collection.update(
                                         { _id :  item._id },
                                         { $set: { recid : j } },
                                         { w : 0}
                                         );
                                 
                                        // Remove the selected attribute
                                        collection.update(
                                        {_id: item._id},
                                        {$set: { selected : false}},
                                        {w: 0}
                                        );
 
                                    }
                                }
                                else
                                {
                                    var result_data = {
                                        "status": "sucess",
                                        "total": array.length,
                                        "records": array
                                    };

                                    //response.write(job);         
                                    console.log(result_data);

                                    response.write(JSON.stringify(result_data));
                                    response.end();


                                }
                            });
                        });
                    });
                });

            }

            break;
        case 'save-record':
            {
                console.log("SAVE RECORD");
                mc.connect(dbs, function(err, db) {
                    if (err)
                        throw(err);
                    var collection = db.collection("positions", function(err, collection) {
                        var acti=false;
                        if (request.body.record["active"]==1)
                        {
                            acti=true;
                        }

                        var doc = {_id: request.body.record["_id"],
                            recid: Number(request.body.record["recid"]),
                            active : acti,
                            data: request.body.record
                        };
                        var coord={
                            lat : Number(request.body.record["lat"]),
                            lng : Number(request.body.record["lng"]),
                            acr : Number(request.body.record["acr"])
                        };
                                                
                        doc.data.coords=new Array;
                        doc.data.coords.push(coord);
                        doc.data.points=Number(doc.data.points);
                        collection.save(doc, {w: 1}, function(err, docs) {
                            if (err) {
                                var err_response = {
                                    "status": "error",
                                    "message": err.err
                                };
                                response.write(JSON.stringify(err_response));
                                console.log(err_response);
                                response.end();
                            }
                            else
                            {
                                response.write(JSON.stringify(save_response));
                                response.end("\r\n");
                            }
                        });

                    });
                });

            }

            break;
        case 'delete-records':
            var delsel = request.body.selected;
            console.log("Delete RECORD ,", Number(delsel[0]));
            mc.connect(dbs, function(err, db) {
                if (err)
                    throw(err);
                var collection = db.collection("positions", function(err, collection) {
//                    collection.remove({
//                        "recid" : {"$eq": Number(delsel[0]) }
//                    }, function(err, removed) {
//                        db.close();
//                        console.log(removed);
//                    });

                    collection.remove({
                        recid : Number(delsel[0]) 
                    }, function(err, removed) {
                        //db.close();
                        console.log(removed);
                    });


                });
            });


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

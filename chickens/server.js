

var save_response = {
    "status": "success"
//          "message"   : "Not implemented yet"
};


var dbs = "mongodb://localhost:27017/cc";

var mc = require('mongodb').MongoClient;


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
                ///var second = JSON.stringify(fake); 
                //response.write(second);
                var result_data = {
                    "status": "sucess",
                    "total": 0,
                    "records": {}
                };

                console.log(result_data);

                response.write(JSON.stringify(result_data));
                response.end();

                mc.connect(dbs, function(err, db) {
                    if (err)
                        throw(err);
                    var collection = db.collection("chickens", function(err, collection) {
                        console.log("find!!");

                        var array = new Array();
                        var cursor = collection.find().sort({ _id : 1},function(err, cursor) {
                            var j = 0;
                            cursor.each(function(err, item) {
                                if (item) {
                                    //console.log(item);
                                    if (item.data)
                                    {
                                        //console.log(item.data.record);
                                        item.data.selected=false;
                                        array.push(item.data);
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
                    var collection = db.collection("chickens", function(err, collection) {

                        var doc = {_id: Number(request.body.record["_id"]),
                            recid: request.body.record["recid"],
                            name: request.body.name,
                            data: request.body.record
                        };

                        collection.save(doc, { w: 1} , function(err, docs) {
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
                                response.end();
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
                var collection = db.collection("chickens", function(err, collection) {
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






var save_response = {
    "status": "success"
//          "message"   : "Not implemented yet"
};


var dbs = "mongodb://localhost:27017/cc";

var mc = require('mongodb').MongoClient;

exports.usersAdd= function(username) {
    mc.connect(dbs, function(err, db) {
        if (err)
            throw(err);
            db.collection("users", function(err, userscoll) {
               console.log("Add user!!",username);
               userscoll.find().toArray(function(err, docs) {
                   //console.log(docs);
                   var found=false;
                   for (var ix=0;ix<docs.length;ix++)
                   {
                       if (docs[ix].data.name===username)
                       {
                           found=true;
                       }
                   }
                   
                   if (found===false)
                   {
                        var record = {
                            _id:docs.length+1,
                            name: username,
                            points: 0
                        }
                        var doc = {_id: Number(docs.length+1),
                             recid: docs.length+1,
                             data: record
                         };

                        userscoll.save(doc, {w: 1}, function(err, docs) {
                            
                        });
                    }                   
               });
           });
       });
};

exports.servePosition=function servePosition( socket )
{
        var array = new Array();
        
        mc.connect(dbs, function(err, db) {
        if (err)
            return(array);
            //throw(err);
        var collection = db.collection("positions", function(err, collection) {
            console.log("serving positions");

           
            var cursor = collection.find().sort({ _id : 1},function(err, cursor) {
                var j = 0;
                cursor.each(function(err, item) {
                    if (item) {
                        //console.log(item);
                        if (item.data)
                        {
                            //console.log("serve",item.data);
                            //item.data.selected=false;
                            array.push(item.data);
                            socket.emit('load:coords', item.data);
                        }
                    }
                    else
                    {
                        db.close();
                        console.log("my array",array);
                        return(array);
                    }
                });
            });
        });
    });
    console.log("Async???");
    return(array);
}

exports.savePosition=function(thePos)
{
    console.log("SAVE Positions");
    mc.connect(dbs, function(err, db) {
      if (err)
          throw(err);
          var collection = db.collection("positions", function(err, collection) {

          var doc = {
              _id: thePos.id,
              recid: 0,
              data: thePos
          };

          collection.save(doc, {w: 1}, function(err, docs) {
              if (err) {
                  console.log("failed save",thePos);
              }
              
              db.close();
          });
      });
  });
}


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

    switch (request.body.cmd)
    {
        case 'get-records':
            {
                console.log("Hallelulja");
                ///var second = JSON.stringify(fake); 
                //response.write(second);

                mc.connect(dbs, function(err, db) {
                    if (err)
                        throw(err);
                    var collection = db.collection("users", function(err, collection) {
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
                    var collection = db.collection("users", function(err, collection) {

                        var doc = {_id: Number(request.body.record["_id"]),
                            recid: request.body.record["recid"],
                            name: request.body.name,
                            data: request.body.record
                        };

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
                var collection = db.collection("users", function(err, collection) {
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




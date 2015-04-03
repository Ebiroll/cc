

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
                         var maxNum=docs.length+1;
                         for (var q=0;q<docs.length;q++)
                         {
                             if (maxNum==Number(docs[q]._id))
                             {
                                 maxNum=Number(docs[q]._id)+1;
                             }
                         }

                       
                        var record = {
                            _id:maxNum,
                            name: username,
                            points: 0
                        }
                        var doc = {_id: Number(maxNum),
                             recid: Number(maxNum),
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

updatePoints=function(thePos)
{
    console.log("update points", thePos);
    mc.connect(dbs, function(err, db) {
        //throw(err);
        db.collection("users", function(err, users) {
            users.find({"data.name": thePos.id}).toArray(function(err, docs) {
                var doc = docs[0];
                doc.data.points = Number(doc.data.points )+1; 
                users.save(doc, {w: 1}, function(err, result) {
                    if (err) {
                        console.log("failed save", thePos);
                    }

                    db.close();
                });
            });
        });
    })
    
}


checkForChickens=function(thePos,socket)
{
        console.log("give me chickens",thePos);
        mc.connect(dbs, function(err, db) {
            //throw(err);
            db.collection("chickens", function(err, chickens) {
            console.log("checking chickens");
            chickens.find().sort({ _id : 1},function(err, cursor) {
                var j = 0;
                cursor.each(function(err, item) {
                    if (item) console.log("found---",item.data.active);                        
                    if (item) {
                        if ( Number(item.data.active) === 1) {
                            //console.log(item);
                            if ((Math.abs(Number(thePos.coords[0].lat)-Number(item.data.lat))<0.001) &&
                                (Math.abs(Number(thePos.coords[0].lng)-Number(item.data.lng))<0.001)) {
                                socket.emit('found:chicken', item.data.smallimg);
                                console.log("found@@@@@@@@",item.data);
                                updatePoints(thePos);
                                item.data.active="0";

                                chickens.save(item, {w: 1}, function(err, docs) {
                                              if (err) {
                                                  console.log("failed save",item);
                                              }                           
                                          });
                            }
                        }
                                                
                    }
                    else
                    {   
                        db.close();
                        // No chickens found :-P
                    }
                });
            });
        });
    });
}


exports.savePosition=function(thePos,socket)
{
    console.log("SAVE Positions");
    checkForChickens(thePos,socket)
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
                        var cursor = collection.find().sort({ "data.points" : -1},function(err, cursor) {
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
                console.log("Hallelulja");
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
                                    if (item.data)
                                    {
                                        console.log(item.data.record);
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

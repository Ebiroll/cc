var topconfig = {
		name: 'toolbar',
		items: [
                        { type: 'break', id: 'break0' },                    
			{ type: 'menu',   id: 'forms', caption: 'Forms', icon: 'fa-table', items: [
				{ text: 'Users', icon: 'fa-camera' }, 
				{ text: 'Chickens', icon: 'fa-picture' }, 
                           	{ text: 'Positions', icon: 'fa-picture' }, 
   
			]},
			{ type: 'break', id: 'break1' },
                        { type: 'button',  id: 'hide',  caption: 'Hide Toolbar' },  // , img: 'icon-save'
                        //{ type: 'break', id: 'break3' },
                        //{ type: 'button',  id: 'hideForm',  caption: 'Form  On/Off' },  // , img: 'icon-save'
			{ type: 'spacer' },
			{ type: 'button',  id: 'help',  caption: 'Help', icon: 'fa-flag' }
		],
                onClick : function(event) 
                {
                    if (event.target=="tasks")
                    {
                       window.location = '/tasks/tasks.html';
                    }
                    
                    if (event.target=="hideForm")
                    {
                      w2ui.layout.hide('main',true);
                      w2ui.layout.hide('left',true);
                    }
                    if (event.target=="hide")
                    {
                      w2ui.layout.hide('top',false);
                    }
                },
                menuClick: function (id,menu_index,event) {
			//console.log('Target: '+ event.target, event);
                        //console.log('id: '+ id );
                        console.log('index: ' , menu_index );
                        console.log('id: ' );
                        console.log(id);
                        //console.log('subItem.id: ' );
                        console.log(id.subItem);
                        
                        // 1.4 version
                        if (id.subItem)
                        {
                            switch (id.subItem.id)
                            {
                                case "Users":
                                    menu_index="0";
                                    break;
                                    
                                 case "Chickens":
                                    menu_index="1";
                                    break;
      
                                 case "Positions":
                                    menu_index="2";
                                    break;
                                    
                            }
                        }
                        
                         
                       var selectedId=id;
                       if (id.item && id.item.id)
                       {
                           selectedId=id.item.id;
                       }
                       if (selectedId=="forms" )
                       {
                               switch (menu_index)
                            {
                                case "0":
                                    window.location = '/users/users.html';
                                    break;

                                case "1":
                                    window.location = '/chickens.html';
                                    break;
         
                                case "2":
                                    window.location = '/users/position.html';
                                    break;
         
                                    
                            }
                       }
                                               
		}
 };	

var topconfig = {
		name: 'toolbar',
		items: [
                        { type: 'break', id: 'break0' },                    
			{ type: 'menu',   id: 'forms', caption: 'Forms', icon: 'fa-table', items: [
				{ text: 'Users', icon: 'fa-camera' }, 
				{ text: 'Chickens', icon: 'fa-picture' }, 
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
                        //console.log('index: '+ menu_index );
                       if (id=="forms")
                       {
                               switch (menu_index)
                            {
                                case "0":
                                    window.location = '/users/users.html';
                                    break;

                                case "1":
                                    window.location = '/chickens.html';
                                    break;
         
                                    
                            }
                       }
                                               
		}
 };	

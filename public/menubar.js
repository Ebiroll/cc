var topconfig = {
		name: 'toolbar',
		items: [
                        { type: 'button',  id: 'tasks',  caption: 'Tasks' },  // , img: 'icon-save'
                        { type: 'break', id: 'break0' },                    
			{ type: 'menu',   id: 'forms', caption: 'Forms', icon: 'fa-table', items: [
				{ text: 'Sites', icon: 'fa-camera' }, 
				{ text: 'Customer', icon: 'fa-picture' }, 
				{ text: 'Campaign', icon: 'fa-glass' },
      				{ text: 'DDC', icon: 'fa-glass' }                            
			]},
			{ type: 'break', id: 'break1' },
                        { type: 'menu',   id: 'report', caption: 'Reports', icon: 'fa-table', items: [
				{ text: 'Capacity' }, 
      				{ text: 'Capacity for all' }, 
				{ text: 'Calendar', icon: 'fa-picture' }, 
				{ text: 'Map', icon: 'fa-glass' }                                
			]},
                        { type: 'break', id: 'break2' },
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
                                    window.location = '/site/sites.html';
                                    break;

                                case "1":
                                    window.location = '/customer/customer.html';
                                    break;
     
                                case "2":
                                    window.location = '/campaign/campaign.html';
                                    break;
                                
                                case "3":
                                    window.location = '/ddc/ddc.html';
                                    break;
     
                                    
                            }
                       }

                        

                        if (id=="report")
                        {
                            switch (menu_index)
                            {
                                case "0":
                                    window.location = '/capacity_overview/overview.html';
                                    break;

                                case "1":
                                    window.location = '/capacity_overview/all.html';
                                    break;                                    
                                
                                case "2":
                                    window.location = '/calendar/calendar.html';
                                    break;
                                    
                                case "3":
                                    window.location = '/site/map.html';
                                    break;
      
     
                            }
                        //    console.log(event.item);
                        }
                        
		}
 };	

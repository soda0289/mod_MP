

function browser(parent_div, music_ui_ctx){
	this.tables = [];//This is an array of panes/tables that will use to create playlists.
	
	//Wipe ids from query parameters object
	this.wipe_ids = function(query_parameters){
		query_parameters.artist_id = [];
		query_parameters.album_id = [];
		query_parameters.song_id= [];
	}
	
	//add ids(artist/album) from all tables in music browser
	//to query parameters object
	this.add_ids = function(query_parameters){
		for(index in this.tables){
			var curr_table = this.tables[index];
			var table_id_type = curr_table.query.parameters.type.substring(0, curr_table.query.parameters.type.length -1) + "_id";
			query_parameters[table_id_type] = query_parameters[table_id_type].concat(curr_table.selected_ids);
		}
	}
	

	this.new_playlist_from_selection = function(){
		var parameters = new query_parameters("songs");

		this.add_ids(parameters);
		
		parameters.sort_by = "song_title";
		parameters.num_results = 1000;
		
		var new_playlist = new playlist(music_ui_ctx.domain, parameters);
		music_ui_ctx.playlist_tabs_if.add_tab("new 1", new_playlist);
		/*
		this.albums_table.clear();
		this.artists_table.clear();
		this.albums_query.parameters.artist_id = [];
		this.albums_query.load();
		this.artists_query.parameters.album_id = [];
		this.artists_query.load();
		*/
	};
	
	
	this.click_row = function(){
		var browser = this;
		return function(data_elem, table_p){
			return function(event){
				var right = 0;
				//Check if right click
				if ("which" in event){
					// Gecko (Firefox), WebKit (Safari/Chrome) & Opera
					right = (event.which === 2); 
				}else if ("button" in event){  // IE, Opera 
					right = (event.button === 1); 
				}
				
				if(right){
					if (event.stopPropagation){
			            event.stopPropagation();
					}
			        event.cancelBubble = true;
					aab.new_playlist_from_selection();
				}else{
					//Select the table row
					var is_selected = table_p.select_row(data_elem.index);
					
					//Add the artist/album ids to browser
					var passing_type = table_p.query.parameters.type.substring(0, table_p.query.parameters.type.length -1) + "_id";
					var passing_id = data_elem[passing_type];
					
					//If not selected add to id list for music browser
					if(is_selected === 0){
						table_p.selected_ids.push(passing_id); 
					}else{
						var index = table_p.selected_ids.indexOf(passing_id);
						if(index !== -1){
							table_p.selected_ids.splice(index, 1);
						}
					}
					
					//Pass down the new selected item to adjacent tables
					//We change the query of the table next to its right
					for(var curr_table = table_p.next; curr_table !== undefined; curr_table = curr_table.next){
						var query = curr_table.query;
						var selections = query.parameters[passing_type];
						//Remove selected item
						if(is_selected !== 0){
							var index = selections.indexOf(passing_id);
							if(index !== -1){
								selections.splice(index, 1);
							}
						//Add selected item to query
						}else{
							selections.push(passing_id);	
						}
						curr_table.clear();
						//Reset query
						query.reset();
						//Wipe query ids
						music_ui_ctx.browser_if.wipe_ids(query.parameters)
						//Add current ids to query
						music_ui_ctx.browser_if.add_ids(query.parameters);
						query.load();
					}
				
				}
				return false;
			};
		};
		
	};
	
	this.sort = function(){
		
	}
	
	this.create_table = function(){
		var win = new floating_box("create_table_box","50%","50%","300px", "300px");
		win.innerHTML("Pick DB table:");
		
		var select_db_table = document.createElement("select");
		select_db_table.id = "select_db_table";
		
		var option_albums = document.createElement("option");
		option_albums.innerHTML = "Albums";
		option_albums.value = "album";
		select_db_table.appendChild(option_albums);
		
		var option_artists = document.createElement("option");
		option_artists.innerHTML = "Artists";
		option_artists.value = "artist";
		select_db_table.appendChild(option_artists);
		
		var option_songs = document.createElement("option");
		option_songs.innerHTML = "Songs";
		option_songs.value = "song";
		select_db_table.appendChild(option_songs);
		
	
		
		
		
		win.appendChild(select_db_table);
		
		var ok_button = document.createElement("input");
		ok_button.type = "button";
		ok_button.value = "OK";
		ok_button.onclick = function(event){
			var select_db_table = document.getElementById("select_db_table");
			var table = select_db_table[select_db_table.selectedIndex].value;
			music_ui_ctx.browser_if.add_table([{"header" : table,"friendly_name" : table + "_name"}], table+"s");
			
			//check if valid
			
			//delete window
			var create_table_box = document.getElementById("create_table_box");
			create_table_box.parentNode.removeChild(create_table_box);
		}
		
		win.appendChild(ok_button);
		win.show();
	}
	
	this.add_table = function(columns, query_type){
		
		var new_table = new table(columns, this.click_row(new_table), this.sort);
		var query_param = new query_parameters(query_type);
		query_param.num_results = "750";
		this.add_ids(query_param);
		
		new_table.query = new music_query(music_ui_ctx.domain, query_param,new_table.add_rows_cb);
		new_table.query.sort_by = "artist_name";
		
		
		if(this.tables.length > 0){
			this.tables[this.tables.length - 1].next = new_table;
		}
		this.tables.push(new_table);
		
		new_table.table_div.style.display = "inline-block";
		new_table.table_div.style.minWidth = "180px";
		new_table.table_div.style.height = "200px";
		new_table.query.load();
		this.div.appendChild(new_table.table_div);
	};
	
	//Create Browser Div
	var browser_div = document.createElement("div");
	browser_div.id = "browser_div";
	this.div = browser_div;
	
	//Create Table Button
	var create_table_button = document.createElement("div");
	create_table_button.className = "create_table";
	create_table_button.innerHTML = "+";
	create_table_button.onclick = this.create_table;
	browser_div.appendChild(create_table_button);
	
	
	
	
	parent_div.appendChild(this.div);
}
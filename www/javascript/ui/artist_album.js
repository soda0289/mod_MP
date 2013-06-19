

function browser(parent_div, music_ui_ctx){
	this.tables = [];//This is an array of panes that will use to select playlists.
	
	this.click = function(){
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
					var selected = null;
					
					selected = table_p.select_row(data_elem.index);
					
					//Pass down the new selected item to ajacent tables
					//We change the query of the table next to our right
					if(table_p.next !== undefined){
						var passing = table_p.query.parameters.type.substring(0, table_p.query.parameters.type.length -1) + "_id";
						var query = table_p.next.query;
						var selections = query.parameters[passing];
						//Remove selected item
						if(selected !== 0){
							var index = selections.indexOf(data_elem[passing]);
							if(index !== -1){
								selections.splice(index, 1);
							}
						//Add selected item to query
						}else{
							selections.push(data_elem[passing]);	
						}
						table_p.next.clear();
						query.reset();
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
		
		var new_table = new table(columns, this.click(new_table), this.sort);
		var query_param = new query_parameters(query_type);
		query_param.num_results = "750";
		
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

/*
function artist_album_browser(parent_div, music_ui_ctx){
	this.artists = [];
	this.albums = [];
	
	this.artist_column = [{"header" : "Artist","friendly_name" : "artist_name"}];
	
	this.new_playlist_from_selection = function(){
		var parameters = new query_parameters("songs");
		parameters.artist_id = this.albums_query.parameters.artist_id.concat(this.artists_query.parameters.artist_id);
		parameters.album_id = this.artists_query.parameters.album_id.concat(this.albums_query.parameters.album_id);
		parameters.sort_by = "song_title";
		parameters.num_results = 1000;
		
		var new_playlist = new playlist(music_ui_ctx.domain, parameters);
		music_ui_ctx.playlist_tabs_if.add_tab("new 1", new_playlist);
		
		this.albums_table.clear();
		this.artists_table.clear();
		this.albums_query.parameters.artist_id = [];
		this.albums_query.load();
		this.artists_query.parameters.album_id = [];
		this.artists_query.load();
		
	};
	
	this.click = function(aab, type){

		return function(data_elem){
			
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
					var selected = null;
					var ids = aab[type];
					//We change the query of the table next to our right
					var query = aab[type + "s_query"];		
					//table is located at artist_albums_browser.albums_table
					selected = aab[type + "s_table"].select_row(data_elem.index);
					if(selected !== 0){
						var index = ids.indexOf(data_elem[type + "_id"]);
						if(index !== -1){
							ids.splice(index, 1);
						}
					}else{
						query.parameters.artist_id.push(data_elem[type + "_id"]);
					}
					aab.albums_table.clear();
					query.reset();
					query.load();
				
				}
				return false;
			};
		};
		
	};
	
	//Double click adds to current playlist except All
	this.artists_dblclick = function(){
		
	};
	
	this.artists_table = new table(this.artist_column, this.click(this, "artist"));


	
	this.album_column = [{"header" : "Albums","friendly_name" : "album_name"}, {"header" : "Year","friendly_name" : "album_id"}];
	
	this.album_click = function(aritst_album_browser){
		//Change artist table with album artist
		return function(album){
			var query = aritst_album_browser.artists_query;
			return function(event){
				aritst_album_browser.albums_table.select_row(album.index);
				query.parameters.album_id.push(album.album_id);
				aritst_album_browser.artists_table.clear();
				query.load();
			};
		};
		
	}(this);
	
	this.sort_albums = function(col){
		//Sort by column change ascending or descending
		return function(){
			this.albums_query.parameters.sort_by = col_fname;
			this.albums_query.parameters.inverted = table_obj.query.parameters.inverted ? 0:1;
			this.albums_query.reset();
			this.albums_query.load();
		}
		
	}
	
	this.albums_table = new table(this.album_column, this.album_click, this.sort_albums);
	
	this.artist_parameters = new query_parameters("artists");
	this.artist_parameters.num_results = "750";
	
	this.artists_query = new music_query(music_ui_ctx.domain, this.artist_parameters,this.artists_table.add_rows_cb);
	this.artists_query.sort_by = "artist_name";
	
	this.album_parameters = new query_parameters("albums");
	this.album_parameters.num_results = "750";
	
	this.albums_query = new music_query(music_ui_ctx.domain, this.album_parameters, this.albums_table.add_rows_cb);
	this.albums_query.sort_by = "album_name";
	
	var artist_album_div = document.createElement("div");
	artist_album_div.style.minHeight = "200px";
	
	
	
	this.artists_table.table_div.style.display = "inline-block";
	this.artists_table.table_div.style.width = "50%";
	this.artists_table.table_div.style.height = "200px";
	artist_album_div.appendChild(this.artists_table.table_div);
	

	this.albums_table.table_div.style.display = "inline-block";
	this.albums_table.table_div.style.width = "50%";
	this.albums_table.table_div.style.height = "200px";
	artist_album_div.appendChild(this.albums_table.table_div);
	
	
	
	this.div = artist_album_div;
	
	this.artists_query.load();
	this.albums_query.load();
	
	parent_div.appendChild(this.div);
	
	this.artists_table.win_resize();
	this.albums_table.win_resize();
}

*/

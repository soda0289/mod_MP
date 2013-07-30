

function playlist(domain, parameters){
	
	//Default shuffled to false
	this.shuffled = false;
	
	//Default Columns
	this.song_columns = 
	[{
		"header" : "Disc#",
		"friendly_name" : "disc_no",
		"width" : "70px",
		"search" : 0,
		"order" : "+"
	},{
		"header" : "Track#",
		"friendly_name" : "track_no",
		"width" : "70px",
		"search" : 0,
		"order" : "+"
	},{
		"header" : "Title",
		"friendly_name" : "song_title",
		"search" : 1
	},{
		"header" : "Artist",
		"friendly_name" : "artist_name",
		"search" : 1
	},{
		"header" : "Albums",
		"friendly_name" : "album_name",
		"search" : 1,
		"order" : "+"
	}];

	this.sort = 
	[{
		"friendly_name":"album_name", 
		"order" : "+"
	},{
		"friendly_name":"disc_no",
		"order" : "+"
	},{
		"friendly_name":"track_no",
		"order" : "+"
	}];
	
	//Callback when songs are added to playlist
	this.song_added = function(playlist, search){
		return function(new_songs){
			//Add rows to table if we are not searching
			if(search === true  || playlist.search_results === undefined){
				playlist.songs_table.add_rows_cb(new_songs);
			}
		};
	};
	
	//Get playlist index from song_id
	this.get_playlist_index = function(song_id){
		//Mapped array of song ids
		var song_ids = this.songs.map(function(song) {
			return song.song_id; 
		});
		
		return song_ids.indexOf(song_id);
	};
	
	this.change_playlist_songs = function(new_songs_array){
		var current_song = this.songs[this.player.playing_index];
		
		//Set songs to new array
		this.songs = new_songs_array;
		//Update playing index
		this.player.playing_index = this.get_playlist_index(current_song.song_id);
		
		//Begin decoding next 4 songs
		this.player.decode_next_songs(4);
	};
	
	//Shuffle Playlist
	this.shuffle = function (){
		//Create temp array of shuffled list
		this.shuffled_playlist = [];
		
		//Fisher–Yates shuffle, as implemented by Durstenfeld
		this.shuffled_playlist[0] = this.songs[0];
		for(var i = 1;i < this.songs.length; i++){
			//Random number from [0, i]
			var random = Math.floor(Math.random() * (i + 1));
			this.shuffled_playlist[i] = this.shuffled_playlist[random];
			if(this.shuffled_playlist[i] !== undefined){
				this.shuffled_playlist[i].shuffled_index = i;
			}
			this.shuffled_playlist[random] = this.songs[i];
			this.shuffled_playlist[random].shuffled_index = random;
		}
		//Save current playlist
		this.unshuffled_playlist = this.songs;
		
		//Set playlist songs to shuffled one
		this.change_playlist_songs(this.shuffled_playlist);
		this.shuffled = true;
	};
	
	//This function will update the table indexs when we change
	//them after a sort
	//It takes new_tindexs which is the songs array with the
	//updated table indexs
	this.update_table_indexs = function(new_tindexs){
		this.songs.forEach(function(song){
			for(var i in new_tindexs){
				var nsong = new_tindexs[i];
				if(nsong.song_id === song.song_id){
					song.table_index = nsong.table_index;
					new_tindexs.splice(i,1);
					//Break out of function
					return;
				}
			}
			console.error("ERROR: \n playlist (update_table_indexs): Didnt find song in new_tindexs. WTF");
		});
	};
	
	//On song click
	this.song_click = function(plist){
		return function(song, table_p){
			return function(event){
				if(plist.player === undefined || plist.player === null){
					console.error("ERROR: \n Playlist (song_click):no player set for this playlist");
				}else{
					//Update player playlist pointer
					plist.player.playlist = plist;
					
					//Get song from playlist
					var s = plist.songs[plist.get_playlist_index(song.song_id)];
					plist.player.audio_obj.play_song(s);
				}
			};
		};
	}(this);
	
	//Sort playlist in order of arrows clicked
	this.sort_playlist = function(plist){
		return function (this_table, col_fname, col_order){
			return function(event){
				var found = false;
				for(var s_col in plist.sort){
					var sort_col = plist.sort[s_col];
					if(sort_col.friendly_name == col_fname){
						if(col_order === '+' || col_order === '-'){
							sort_col.order = col_order;
						}else{
							plist.sort.splice(s_col,1);
						}
						found = true;
						//Break out of function as we have made
						//the nesecary changes. I can't spell
					}

				}
				if(found === false){
					plist.sort.push({"friendly_name" : col_fname, "order" : col_order});
				}

				//If we are searching do not change the playlist songs array
				if(typeof (plist.search_results) === "undefined"){
					plist.query.parameters.sort_by = plist.sort;
					this_table.clear();
					plist.query.reset();
					plist.query.onComplete = function(){
						if(plist.shuffled === false){
							//Change playlist songs array and update playing index
							plist.change_playlist_songs(plist.query.results.songs);
						}else{
							//Remap the table index to the new sorted playlist
							plist.update_table_indexs(plist.query.results.songs);
						}
						//Update current song since its indexs(table and suffled) have changed after sorting
						var current_song = plist.songs[plist.player.playing_index];
						plist.player.change_song(current_song);
					};
					plist.query.load();
				}else{
					plist.search_query.parameters.sort_by = plist.sort;
					this_table.clear();
					plist.search_query.reset();
					plist.search_query.load();
				}

			};
		};
	}(this);
	
	//When the search box changes
	this.search_box_change = function(playlist){
		return function(table, column, search_textbox){
			return function(event){
				//Check if search query has a length
				if(search_textbox.value.length > 0){
					//Add asterkis to search within words
					var search_string = '*' + search_textbox.value + '*';
					//Create search result array
					playlist.search_results = [];
					
					playlist.search_query.parameters[column.friendly_name] = search_string;
					playlist.search_query.reset();
					table.clear();
					playlist.search_query.load();
				}else{
					playlist.search_query.parameters[column.friendly_name] = null;
					
					//Check if all search boxes are cleared and reset
					var is_empty = table.columns.every(function (col){
						if(col.search === 1){
							//Check if parameter is not set
							return !Boolean(playlist.search_query.parameters[col.friendly_name]);
						}else{
							return true;
						}
					});
					if(is_empty === true){
						playlist.search_results = undefined;
						table.clear();
						playlist.query.reset();
						playlist.query.onComplete = function (){
							var current_song = playlist.songs[playlist.player.playing_index];
							
							//Set new table index based on playlist
							
							playlist.player.change_song(current_song);
						};
						playlist.query.load();
					}
					
				}
			};
		};
	}(this);

	this.insert_into_div = function(div){
		div.appendChild(this.div);
		//Resize header
		//this should be removed
		this.songs_table.win_resize();
	};
	
	//Create table to show songs
	this.songs_table = new table(this.song_columns, this.song_click,this.sort_playlist, this.search_box_change);
	this.songs_table.table_div.style.width = "100%";
	this.songs_table.table_div.className = "playlist";
	
	this.div = this.songs_table.table_div;
	

	//Setup playlist songs query

	this.query = new music_query(domain, parameters, this.song_added(this));
	//Is there a name for this technique
	//Am I just currying this???
	this.query.onComplete = function(playlist){
		return function(){
			//When query complete copy query results
			//to playlist
			playlist.songs = playlist.query.results.songs;
		};
	}(this);
	
	//Setup playlist search query
	this.search_query = new music_query(domain, parameters.clone(), this.song_added(this, true));
	this.search_query.onComplete = function(playlist){
		return function(){
			//Copy search results to search query
			playlist.search_results = playlist.search_query.results.songs;
		};
	}(this);
	if(this.query !== null){
		this.query.load();
	}
}

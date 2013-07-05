

function playlist(domain, parameters){
	
	
	
	this.songs = [];
	
	this.song_columns = [
 	                {
 	                	"header" : "Disc#",
						"friendly_name" : "disc_no",
						"width" : "70px",
						"search" : 0
					},{
						"header" : "Track#",
						"friendly_name" : "track_no",
						"width" : "70px",
						"search" : 0
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
						"search" : 1
					}];
	
	this.song_added = function(playlist){
		
		return function(new_songs){
			//Add rows to table and insert row index into song object
			playlist.songs_table.add_rows_cb(new_songs);
		};
	}(this);
	
	
	this.shuffle = function (){
		//Set playlist songs array to current query results of table
		this.songs = this.songs_table.query.results;
		
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
		
		this.unshuffled_playlist = this.songs;
		this.songs = this.shuffled_playlist;
	};
	
	this.song_click = function(plist){
		
		return function(song){
			return function(event){
				if(plist.player === undefined || plist.player === null){
					console.log("no player set for this playlist");
				}else{
					plist.player.playlist = plist;
					plist.player.audio_obj.play_song(song);
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
	

	
	
	
	this.songs_table = new table(this.song_columns, this.song_click, this.search_box_change);

	this.songs_table.table_div.style.width = "100%";
	this.songs_table.table_div.className = "playlist"
	
	this.div = this.songs_table.table_div;
		
	this.query = this.songs_table.query = new music_query(domain, parameters, this.song_added);

	
	if(this.query !== null){
		this.songs_table.query.load();
	}
}
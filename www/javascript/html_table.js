function print_song_table(music_ui_ctx){
	
	var songs = music_ui_ctx.songs;
	var songs_div = document.getElementById("songs");
	//Create song table if it doesn't exsist
	var songs_table = document.getElementById("songs_table");
	if (songs_table == null){
		songs_table = document.createElement('table');
		songs_table.id = "songs_table";
		songs_table.style.width = "100%";
		songs_div.appendChild(songs_table);
	}
	
	for(var i = songs_table.rows.length; i < music_ui_ctx.songs.length; i = songs_table.rows.length){
		var bgcolor = (i%2 == 0)? "#222222" : "#666666";

		var new_row = document.createElement('tr');
		new_row.style.backgroundColor= bgcolor;
		new_row.style.color = "orange";
		
		new_row.onclick = (function (){
			var id = i;
			
			return function(){
				play_song(id, music_ui_ctx);
			}
			
		})(i, music_ui_ctx);
		
		var new_col = new Array();
		new_col[0] = document.createElement('td');
		new_col[0].style.width = "33%";
		new_col[0].innerHTML =  songs[i].song_title;
		new_row.appendChild(new_col[0]);
		new_col[1] = document.createElement('td');
		new_col[1].style.width = "33%";
		new_col[1].innerHTML =  songs[i].artist_name;
		new_row.appendChild(new_col[1]);
		new_col[2] = document.createElement('td');
		new_col[2].style.width = "33%";
		new_col[2].innerHTML =  songs[i].album_name;
		new_row.appendChild(new_col[2]);
		
		//newcontent.innerHTML += "Title: " + myObj.songs[i].title + "  Artist: " + myObj.songs[i].Artist +"  Album:  "+myObj.songs[i].Album;
		songs_table.appendChild(new_row);
	}
}

function print_artists(artists, queries){
	var artists_div = document.getElementById("artists");
	for(var i = 0; i < parseInt(artists.length, 10); i++){
		
		var bgcolor = (i%2 == 0)? "#222222" : "#666666";

		var newcontent = document.createElement('div');
		newcontent.style.backgroundColor= bgcolor;
		newcontent.style.color = "orange";
		newcontent.innerHTML += artists[i].artist_name;
		newcontent.onclick = (function (){
			var artist_id = artist.artist_id
			return function(){
				//clear albums
				var albums_div = document.getElementById('albums');
				var songs_div = document.getElementById('songs');
				//Stop running queries
				queries.songs_query.running = 0;
				queries.albums_query.running = 0;
				albums_div.innerHTML = "";
				songs_div.innerHTML = "";
				queries.songs = new Array();
				queries.albums = new Array();
				
				//Create two new queries
				var songs_query = new music_query("mp.attiyat.net", 1000, "songs", "song_title", artist_id, 0, 0,print_song_table);
				var albums_query = new music_query("mp.attiyat.net", 100, "albums", "album_name", artist_id, 0, 0,print_albums);
				
				
				//Update ui
				queries.songs_query = songs_query;
				queries.albums_query = albums_query;

				
				load_query(songs_query, queries);
				load_query(albums_query, queries);
				
			}
			
		})(artist = artists[i], queries);
		artists_div.appendChild(newcontent);
	}
}

function print_albums(albums, queries){
	var albums_div = document.getElementById("albums");
	for(var i = 0; i < parseInt(albums.length, 10); i++){
		
		var bgcolor = (i%2 == 0)? "#222222" : "#666666";

		var newcontent = document.createElement('div');
		newcontent.style.backgroundColor= bgcolor;
		newcontent.style.color = "orange";
		newcontent.innerHTML += albums[i].album_name;
		newcontent.onclick = (function (){
			var album_id = album.album_id;
			var artist_id = album.artist_id;
			return function(){
				var songs_div = document.getElementById('songs');
				//Stop running quieres
				queries.songs_query.running = 0;
				songs_div.innerHTML = "";
				queries.songs = new Array();
				var songs_query = new music_query("mp.attiyat.net", 1000, "songs", "song_title", artist_id, album_id, 0,print_song_table);
				
				//Update ui
				queries.songs_query = songs_query;
				
				load_query(songs_query, queries);
			}
			
		})(album = albums[i], queries);
		albums_div.appendChild(newcontent);
	}
}
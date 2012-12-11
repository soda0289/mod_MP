function print_song_table(songs, queries){
	
	songs_div = document.getElementById("songs");
	//Create song table if it doesn't exsist
	var songs_table = document.getElementById("songs_table");
	if (songs_table == null){
		songs_table = document.createElement('table');
		songs_table.id = "songs_table";
		songs_table.style.width = "100%";
		songs_div.appendChild(songs_table);
	}
	
	for(var i = 0; i < parseInt(songs.length, 10); i++){
		var bgcolor = (i%2 == 0)? "#222222" : "#666666";

		var new_row = document.createElement('tr');
		new_row.style.backgroundColor= bgcolor;
		new_row.style.color = "orange";
		
		new_row.onclick = (function (){
			var id = i;
			
			return function(){
				play_song(id, queries);
			}
			
		})(i, queries);
		
		var new_col = new Array();
		new_col[0] = document.createElement('td');
		new_col[0].style.width = "33%";
		new_col[0].innerHTML =  songs[i].title;
		new_row.appendChild(new_col[0]);
		new_col[1] = document.createElement('td');
		new_col[1].style.width = "33%";
		new_col[1].innerHTML =  songs[i].Artist;
		new_row.appendChild(new_col[1]);
		new_col[2] = document.createElement('td');
		new_col[2].style.width = "33%";
		new_col[2].innerHTML =  songs[i].Album;
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
		newcontent.innerHTML += artists[i].name;
		newcontent.onclick = (function (){
			var artist_id = artist.id
			return function(){
				//clear albums
				var albums_div = document.getElementById('albums');
				var songs_div = document.getElementById('songs');
				//Stop running queries
				queries.songs_query.running = 0;
				queries.albums_query.running = 0;
				albums_div.innerHTML = "";
				songs_div.innerHTML = "";
				
				//Create two new queries
				var songs_query = new music_query("mp", 1000, "songs", "+titles", artist_id, 0, print_song_table);
				var albums_query = new music_query("mp", 100, "albums", "+albums", artist_id, 0, print_albums);
				
				
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
		newcontent.innerHTML += albums[i].name;
		newcontent.onclick = (function (){
			var album_id = album.id;
			return function(){
				var songs_div = document.getElementById('songs');
				//Stop running quieres
				queries.songs_query.running = 0;
				songs_div.innerHTML = "";
				var songs_query = new music_query("mp", 1000, "songs", "+titles", 0, album_id, print_song_table);
				
				//Update ui
				queries.songs_query = songs_query;
				
				load_query(songs_query, queries);
			}
			
		})(album = albums[i], queries);
		albums_div.appendChild(newcontent);
	}
}
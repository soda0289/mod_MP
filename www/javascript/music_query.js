
//Music query object
function music_query(hostname, num_results, type, sort_by,artist_id,album_id, song_id,print_results_function){
	this.running = 1;
	this.count = 0;
	this.num_results = num_results;
	
	this.hostname = hostname;
	this.type = type;
	this.sort_by = sort_by;
	this.artist_id = artist_id;
	this.album_id = album_id;
	this.song_id = song_id;
	this.source_id;
	
	this.source_type = "ogg";
	this.url;
	
	
	this.set_url = function (){
		this.upper = (this.count * this.num_results).toString();
		this.url = "http://" + this.hostname + "/music/" + this.type;
		//Check if artist_id or album_id is set
	
		if (this.album_id > 0) {
			this.url += "/album_id/" + album_id;
		}
		if (this.artist_id > 0) {
			this.url += "/artist_id/" + artist_id;
		}
		if (this.song_id > 0) {
			this.url += "/song_id/" + song_id;
		}
		if (this.source_id > 0) {
			this.url += "/source_id/" + this.source_id;
		}
		if (this.sort_by !== null) {
			this.url += "/sort_by/" + sort_by;
		}
		if (type === "sources"){
			if(this.source_type !== null){
				this.url += "/source_type/" + this.source_type;
			}
		}
		if (type === "transcode"){
			this.url += "/output_type/" + this.source_type;
		}
		if(num_results > 0){
			this.url += "/limit/" + num_results;
			if(this.upper > 0){
				this.url += "/offset/" + this.upper;
			}
		}
	}

	this.print_results = print_results_function;
}

function load_query(music_query, music_ui_ctx){

		music_query.set_url();
		var upper = music_query.num_results;
	
		if (window.XMLHttpRequest){// code for IE7+, Firefox, Chrome, Opera, Safari
			var xmlhttp=new XMLHttpRequest();
		} else {
			//should really error out
		  	return -1;
		}
	
	
		var lower = music_query.count * parseInt(upper,10);
		var url = music_query.url;
		xmlhttp.open("GET",url,false);
		
		//Since each xmlhttp request is an array we pass the index of it to the new function
		xmlhttp.onreadystatechange=function(){
			//we then must return a function that takes no parameters to satisfy onreadystatechange
			return function(){
				var num_results;
				
				if (xmlhttp.readyState == 4 && xmlhttp.status == 200){
					try{
						var json_object = JSON.parse(String(xmlhttp.responseText), null);
					}catch (err){
						alert("error: " + err + " on request number. URL:" + music_query.url);
						return -1;
					}
					//Is the query running
					//Did the server list any results
					if(music_query.running == 1){
						if (music_query.type == "songs"){
							if (parseInt(json_object.songs.length, 10) > 0){
								music_ui_ctx.songs = music_ui_ctx.songs.concat(json_object.songs);
								music_query.print_results(music_ui_ctx);
							}else{
								music_query.running = 0;
								return 0;
							}
						}else if(music_query.type == "artists"){
							if (parseInt(json_object.artists.length, 10) > 0){
								music_query.print_results(json_object.artists, music_ui_ctx);
								music_ui_ctx.artists = music_ui_ctx.artists.concat(json_object.artists);
							}else{
								music_query.running = 0;
								return 0;
							}
						}else if(music_query.type == "albums"){
							if (parseInt(json_object.albums.length, 10) > 0){
								//Print out query results
								music_query.print_results(json_object.albums, music_ui_ctx);
								//Add results to music ui context
								music_ui_ctx.albums = music_ui_ctx.albums.concat(json_object.albums);
							}else{
								music_query.running = 0;
								return 0;
							}	
						}else if(music_query.type == "sources"){
								//USed internally dont print
								//Add results to music ui context
								music_ui_ctx.sources = json_object.sources;
								music_query.running = 0;
								return 0;
						}else if(music_query.type == "transcode"){
							music_ui_ctx.decoding_job = json_object.decoding_job;
							return 0;
						}
						music_query.count++;
						load_query(music_query, music_ui_ctx);
					}
					
				}
			}
		}(music_query);
		xmlhttp.send();
}

function load_queries(music_ui_ctx){
	load_query(music_ui_ctx.songs_query, music_ui_ctx);
	load_query(music_ui_ctx.albums_query, music_ui_ctx);
	load_query(music_ui_ctx.artists_query, music_ui_ctx);
}
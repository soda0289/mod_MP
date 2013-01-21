

//Music UI context object
function music_ui(songs_query, artists_query, albums_query) {
	this.songs_query = songs_query;
	this.artists_query = artists_query;
	this.albums_query = albums_query;
	
	//Create array that links to each query
	//this is only used in the for statement
	this.queries = new Array(this.songs_query, this.artists_query, this.albums_query);
	
	this.songs = new Array();
	this.albums = new Array();
	this.artists = new Array();
	this.sources = new Array();
	this.decoding_job = null;
	
	this.audio_ele = new Audio();
	this.song_loaded = false;
	this.loaded_source = 0;
	this.playing_song_id = 0;
	
	
}


function loadUI(){
	
	//Create queries for Songs, Artists, and Albums
	var songs_query = new music_query("mp.attiyat.net", 1000, "songs", "song_title", 0, 0, 0,print_song_table);
	var artists_query = new music_query("mp.attiyat.net", 100, "artists", "artist_name", 0, 0, 0,print_artists);
	var albums_query = new music_query("mp.attiyat.net", 100, "albums", "album_name", 0, 0, 0,print_albums);
	
	//Load music ui
	var music_ui_ctx = new music_ui(songs_query, artists_query, albums_query);
	
	load_queries(music_ui_ctx);
}
//note to self
//when you set a varible to a function
//you must not include parenthises
//This will run that function and not pass
//the function as a pointer
window.onload = loadUI;
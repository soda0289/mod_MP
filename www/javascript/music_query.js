function query_parameters(type, objects){
	this.type = type;
	this.num_results = 0;
	this.sort_by = null;
	this.artist_id = [];
	this.album_id = [];
	this.song_id = [];
	this.source_id = [];
	this.album_name = "";
	this.artist_name = "";
	this.song_title = "";
	this.source_type = null;
	this.output_type = null;
	this.objects = objects;
	
	this.clone = function(){
		new_obj = new query_parameters(this.type);
		
		for(var attr in this) {
			if(this.hasOwnProperty(attr)){
				new_obj[attr] = this[attr];
			}
		}
		
		return new_obj;
	};
}


//Music query object
function music_query(hostname, parameters,print_results_function){
	this.count = 0;

	this.print_results = print_results_function;
	this.parameters = parameters;
	this.results = {};
	
	this.hostname = hostname;
	
	this.result = null;
	
	this.url = null;
	
	this.reset = function () {
		this.count = 0;
		this.running = 0;
		this.results = [];
	};

	
	
	this.set_url = function (){
		this.upper = (this.count * this.parameters.num_results).toString();
		this.url = "http://" + this.hostname + "/music/" + this.parameters.type;
		//Check if artist_id or album_id is set
	
		if (this.parameters.album_id instanceof Array && this.parameters.album_id.length && this.parameters.album_id.length > 0) {
			this.url += "/album_id/" + this.parameters.album_id;
		}
		if (this.parameters.artist_id instanceof Array && this.parameters.artist_id.length && this.parameters.artist_id.length > 0) {
			this.url += "/artist_id/" + this.parameters.artist_id;
		}
		if (this.parameters.song_id instanceof Array && this.parameters.song_id.length && this.parameters.song_id.length > 0) {
			this.url += "/song_id/" + this.parameters.song_id;
		}
		if (this.parameters.source_id instanceof Array && this.parameters.source_id.length && this.parameters.source_id.length > 0) {
			this.url += "/source_id/" + this.parameters.source_id;
		}
		if (this.parameters.song_title !== null && this.parameters.song_title !== undefined && this.parameters.song_title.length > 0) {
			this.url += "/song_title/" + this.parameters.song_title;
		}
		if (this.parameters.album_name !== null && this.parameters.album_name !== undefined && this.parameters.album_name.length > 0) {
			this.url += "/album_name/" + this.parameters.album_name;
		}
		if (this.parameters.artist_name !== null && this.parameters.artist_name !== undefined && this.parameters.artist_name.length > 0) {
			this.url += "/artist_name/" + this.parameters.artist_name;
		}
		if (this.parameters.sort_by !== null && this.parameters.sort_by !== undefined && this.parameters.sort_by.length > 0) {
			this.url += "/sort_by/" + this.parameters.sort_by;
		}
		if (this.parameters.type === "sources"){
			if(this.parameters.source_type !== null){
				this.url += "/source_type/" + this.parameters.source_type;
			}
		}
		if (this.parameters.type === "transcode"){
			if(this.parameters.output_type !== null){
				this.url += "/output_type/" + this.parameters.output_type;
			}
		}
		if(this.parameters.num_results > 0){
			this.url += "/limit/" + this.parameters.num_results;
			if(this.upper > 0){
				this.url += "/offset/" + this.upper;
			}
		}
	};
	
	this.load = function (){
		
		this.running = 1;

		this.set_url();
	
		if (window.XMLHttpRequest){
			this.xmlhttp = new XMLHttpRequest();
		} else {
			//should really error out
			return -1;
		}

		var url = this.url;
		this.xmlhttp.open("GET",url,true); //Async
		
		this.xmlhttp.onreadystatechange=function(music_query){
			//we then must return a function that takes no parameters to satisfy onreadystatechange
			return function(){
				music_query.process();
			};
		}(this);
		
		try{
			this.xmlhttp.send();
		}catch(error){
			console.log("Error could send query:" + error);
		}
	};
	
	this.process = function (){
		var json_object;
		
		if (this.xmlhttp.readyState === 4 && this.xmlhttp.status === 200){
			try{
				json_object = JSON.parse(String(this.xmlhttp.responseText), null);
			}catch (err){
				console.error("error: " + err + " on request number. URL:" + this.url);
				
				//Re run query
				this.load();
				return;
			}
			
			//Test json_object
			if(typeof json_object !== "object"){
				return console.error("Error: \n Error with query did not return json object");
			}
			
			//Is the query running
			//Did the server list any results
			
			if("Errors" in json_object && json_object.Errors.length > 0){
				for(var x in json_object.Errors){
					var error = json_object.Errors[x];
					if(error.type === 0){
						console.error("ERROR: \n Server: \nType: " + error.type+ "\nHeader: " + error.header + "\nMessage: " + error.message);
					}else{
						console.log("DEBUG: \n Server: \nType: " + error.type+ "\nHeader: " + error.header + "\nMessage: " + error.message);
					}
				}
			
			}
			var json_length = 0;

			//Convert to array to statisfy loop
			if(!(this.parameters.objects instanceof Array)){
				var no = this.parameters.objects;
				this.parameters.objects = [];
				this.parameters.objects.push(no);
			}

			for(var obj in this.parameters.objects){
				var o = this.parameters.objects[obj];
				var o_name = o.name;
				var o_index = o.index;

				var q_object = json_object[o_name];

				if(q_object !== undefined){
					//Check if we need to make array

					if(!(this.results[o_name] instanceof Array)){
						this.results[o_name] = [];
					}

					//Check if we got more than 1 object
					if(q_object instanceof Array){
						json_length += q_object.length;
						//Concat the two arrays
						this.results[o_name] = this.results[o_name].concat(q_object);
					}else{
						this.results[o_name] = q_object;
						json_length += 1;
					}
				

					//Call the print result callback
					if(typeof(this.print_results) === "function"){
						this.print_results(this.results[o_name], o_name);
					}
				}else{
					json_length = 0;
				}
	

			}
			
			

			//If nothing was returned or if the amount returned is less than num expected stop query
			if (json_length === 0 || this.parameters.num_results > json_length ||
				this.parameters.num_results === 0 //When we request zero results the query dies
				){
				
				this.running = 0;
				if(typeof(this.onComplete) === "function"){
					return this.onComplete();
				}
				
			}else{//Else keep loading query
				this.count++;
				return this.load();
			}
			
		}
	};

}

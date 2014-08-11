
function status_bar(music_ui_ctx){
    
	this.div = document.createElement("div");
	this.div.id = "music_statusbar";
	this.div.style.position = "absolute";
	this.div.style.bottom = "0";
	this.div.style.width = "100%";
		
	this.file_sync_span = document.createElement("span");
	this.file_sync_span.className = "stats";
	this.div.appendChild(this.file_sync_span);

	this.db_stats_span = document.createElement("span");
	this.db_stats_span.className = "stats";
	this.div.appendChild(this.db_stats_span);

	this.update_status = function (sbar){
		return function(data, object_name){
			if(object_name === "dirsync_status"){
				//Reload query until progress is 100%
				if(parseFloat(data.Progress) < 100.0){
					setTimeout(
					function(query){
						return function(){
							query.reset();
							query.load();
						};
					}(sbar.query),music_ui_ctx.refresh_interval);
				}
				sbar.file_sync_span.innerHTML = "Progress: " + data.Progress + " Files Scanned: " + data["Files Scanned"];
			}else{
				sbar.db_stats_span.innerHTML = "Songs: " + data["Songs Count"] + " Artsits: "+ data["Artists Count"] + " Albums: " + data["Albums Count"];
			}
		};
    }(this);


    //Setup query
	var obj = [{"name" : "db_status"},{"name" : "dirsync_status"}];

    this.param = new query_parameters("status",obj);
    this.param.object_name = "dirsync_status";
    this.query = new music_query(music_ui_ctx.domain, this.param, this.update_status);
    this.query.load();
}

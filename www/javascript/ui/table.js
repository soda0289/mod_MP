function table(columns, row_click_cb, sort_click_cb, search_change_cb){
	//Selected id in the current table
	this.selected_ids = [];
	
	var column_header_height = "25px";
	
	this.columns = columns;
	
	this.table_div = document.createElement('div');
	this.table_div.style.position = "relative";
	this.table_div.style.display = "inline-block";
	this.table_div.style.height = "100%";
	this.table_div.style.overflowX = "scroll";
	
	this.clear = function() {
		this.selected_ids = [];
		this.table.innerHTML = "";
	};
	
	this.deselect_row = function(index){
		var row = this.table.rows[index];
		if(row !== undefined && row.selected === 1){
			var classes;
			classes = row.className.split(" ");
			var i = classes.indexOf("selected");
			if(i !=-1){
				classes.splice(i,1);
				row.className = classes.join(" ");
				row.selected = 0;
				return 0;
			}

		}
		return -1;
	};
	
	this.select_row = function(index){
		var row = this.table.rows[index];
		if(!("selected" in row) || row.selected === 0){
			row.className += " selected";
			row.selected = 1;
			return 0;
		}else{
			this.deselect_row(index);
		}
		
		return -1;
	};
	
	this.invert_table = function (){
		
		
	};

	this.sort_click = function(table, column, order){
		//If the current column order equals
		//the one click deselect it the sorting
		//on this column

		return function(event){
			//If the column order and the order pressed are equal
			//then we unhighlight it and set order to null
			//else the opposite order was pressed and we swap the highlighted
			//sort button
			if(column.order === order || (order !== "+" && order !=="-")){
				column.order = "";
				this.style.opacity = "0.4";
			}else{
				//If column.order is set we must unhighlight it
				if(column.order === "+" || column.order === "-"){
					var arrow = table.sort_arrows[column.index][column.order];
					arrow.style.opacity = "0.4";
				}
				column.order = order;
				this.style.opacity = "1";
			}

			return sort_click_cb(table, column.friendly_name, column.order)(event);	
		};
	
	};
	
	this.create_column_header = function (){
		//Use header
		this.header_table = document.createElement('table');
		this.header_table.cellSpacing = "0";
		this.header_table.className = "table_header";
		this.header_table.style.width = "100%";
		this.header_table.style.height = column_header_height;
		
		//Create table header (1 row)
		var head_row = document.createElement('tr');
		head_row.style.cursor = "pointer";

		this.sort_arrows = [];
		
		for(var col_index in this.columns){
			var curr_col = this.columns[col_index];
			curr_col.index = col_index;
			
			var new_col = document.createElement("th");
			
			var col_div = document.createElement("div");
			col_div.style.position = "relative";
			col_div.style.height = column_header_height;
			
			//Get column width
			if(curr_col.width === undefined){
				//Auto should determine best width
				new_col.style.width = "auto";
			}else{
				new_col.style.width = curr_col.width;
			}
			
			//Print Title
			var column_title = document.createElement("div");
			column_title.className = "column_title";
			column_title.innerHTML =  curr_col.header;
			
			//Print search box
			if(curr_col.search !== undefined && curr_col.search !== 0 && search_change_cb !== undefined){
				var column_search  = document.createElement("input");
				
				column_search.type = "text";
				column_search.className = "search_textbox";
				column_search.onkeyup = search_change_cb(this, curr_col, column_search);
				
				column_title.appendChild(column_search);
			}
			
			col_div.appendChild(column_title);
			
			
			var arrows_div = document.createElement("div");
			arrows_div.className = "sort_arrows";
			
			var up_arrow = document.createElement("img");
			up_arrow.className = "sort asc";
			up_arrow.src = "svg/sort_arrow.svg";
			up_arrow.width = "12";
			up_arrow.height = "12";
			if(curr_col.order !== "+"){
				up_arrow.style.opacity = "0.4";
			}

			up_arrow.onclick = this.sort_click(this, curr_col, "+");
			var down_arrow = document.createElement("img");
			down_arrow.className = "sort desc";
			down_arrow.src = "svg/sort_arrow.svg";
			down_arrow.width = "12";
			down_arrow.height = "12";
			down_arrow.style.webkitTransform = "rotate(180deg)";
			if(curr_col.order !== "-"){
				down_arrow.style.opacity = "0.4";
			}
			down_arrow.onclick = this.sort_click(this, curr_col, "-");

			this.sort_arrows[col_index] = [];
			this.sort_arrows[col_index]["+"] = up_arrow;
			this.sort_arrows[col_index]["-"] = down_arrow;

			arrows_div.appendChild(up_arrow);
			arrows_div.appendChild(down_arrow);
			
			col_div.appendChild(arrows_div);
			
			new_col.appendChild(col_div);
			
			head_row.appendChild(new_col);
		}
		
		this.header_table.appendChild(head_row);
		
		this.table_div.insertBefore(this.header_table,this.table_scrollbar);
	};
	
	this.create_table = function(){
		//Div with scrollbars to scroll table
		this.table_scrollbar = document.createElement('div');
		this.table_scrollbar.className = "scrolling_div";
		this.table_scrollbar.addEventListener("scroll",
				function(music_ui_ctx){
					return function(event){
						
					};
					
				}(this),false);
		this.table_scrollbar.style.width = "100%";
		this.table_scrollbar.style.overflowY = "scroll";
		this.table_scrollbar.style.overflowX = "hidden";
		this.table_scrollbar.style.position = "absolute";
		this.table_scrollbar.style.bottom = "0";
		this.table_scrollbar.style.top = column_header_height;
		
		this.table = document.createElement('table');
		this.table.className = "table";
		this.table.style.width = "100%";
		this.table.cellSpacing = "0";

		this.table_scrollbar.appendChild(this.table);
		this.table_div.appendChild(this.table_scrollbar);
		this.create_column_header();
	};
	
	this.create_table();
	
	this.add_rows_cb = function(table_p, table_c,column_list, row_click_cb){
		
		return function(data){
			for(var i = table_c.rows.length; i < data.length; i = table_c.rows.length){
				var data_elem = data[i];
				data_elem.table_index = i;
				
				var new_row = document.createElement('tr');
				var parity = (i%2 === 0) ? "even" : "odd";
				new_row.className = parity;
				new_row.style.cursor = "pointer";
				
				for(var col in column_list){
					var new_col;
					new_col = document.createElement('td');
					//Get column width
					if(column_list[col].width === undefined){
						//Auto should determine best width
						new_col.style.width = "auto";
					}else{
						new_col.style.width = column_list[col].width;
					}
					new_col.innerHTML = data_elem[column_list[col].friendly_name];
					new_row.appendChild(new_col);
				}
				if(row_click_cb !== undefined){
					new_row.onclick = row_click_cb(data_elem, table_p);
				}
				
				table_c.appendChild(new_row);
			}
		};
	}(this, this.table,this.columns, row_click_cb);
	
	this.win_resize = function (table_obj){
		return function(){
		//Why minus one??????? It looks way better with minus one!!! Helps table header line up
		table_obj.header_table.style.width = (table_obj.table.offsetWidth - 1) + "px";
		};
	}(this);
	
	window.addEventListener('resize', this.win_resize);
	

}

(function(a){a.su.Widget("paging",{defaults:{grid:null,displayCtrl:true,maxPageBtnNum:5,displayInfo:true,displayMsg:"",emptyMsg:"",pluginId:"",itemNumPerPage:[5,10,20],currentItemNumPerPage:5,currentPage:0,totalPage:0},create:function(e,c){var d=this;d.each(function(j,n){a.extend(n,e,c);var k='<div class="container widget-container paging-container">';k+='<div class="page-num-container inline">';k+='<div class="button-container inline">';k+='<button type="button" class="button-button page-btn prevoius-page-btn" page-num="prevoius">';k+="<span>&lt;</span>";k+="</button>";k+="</div>";k+='<div class="page-btn-container inline">';k+="</div>";k+='<div class="button-container inline">';k+='<button type="button" class="button-button page-btn next-page-btn" page-num="next">';k+="<span>&gt;</span>";k+="</button>";k+="</div>";k+="</div>";k+='<div class="inline info-container">';if(n.displayCtrl){k+='<div class="inline show-in-one-page-container">';k+="<span>"+a.su.CHAR.PAGING.SHOW+"</span>";k+='<input class="inline show-in-one-page">';k+="<span>"+a.su.CHAR.PAGING.IN_ONE_PAGE+"</span>";k+="</div>"}k+="</div>";k+="</div>";var m=a(k);var g=[];for(var h=0,f=n.itemNumPerPage.length;h<f;h++){var l={name:n.itemNumPerPage[h],value:n.itemNumPerPage[h]};g.push(l)}if(n.displayCtrl){m.find("input.show-in-one-page").combobox({fieldLabel:null,inputCls:"xs",items:g}).combobox("select",n.currentItemNumPerPage)}a(n).append(m);n.isPaging=true});var b=a(c.grid.get(0).store);b.on("ev_datachanged",function(f){d.paging("initPaging")});if(d.get(0).displayCtrl){d.delegate("input.show-in-one-page","ev_change",function(j){var i=a(this).combobox("getValue"),g=d.get(0).grid,h=g.get(0).editor,f=a(g).grid("isEditing");if(f==true){a(h).editor("cancelEdit")}d.get(0).currentItemNumPerPage=i[0];d.paging("initPaging").paging("loadPage")})}d.delegate("button.button-button","mousedown",function(g){g.stopPropagation();g.preventDefault();var f=a(this);f.closest("div.button-container").addClass("clicked")}).delegate("button.button-button","click",function(k){k.stopPropagation();k.preventDefault();var h=a(this),j=null,g=d.get(0).grid,i=g.get(0).editor,f=a(g).grid("isEditing");if(f==true){a(i).editor("shake");return}if(h.hasClass("page-btn")){j=h.attr("page-num");switch(j){case"prevoius":d.paging("goPrev");break;case"next":d.paging("goNext");break;default:d.paging("goToPage",j)}}});return d},initPaging:function(n){var n=this,i=n.get(0),d=i.currentItemNumPerPage,k=i.currentPage,c=i.grid,m=c.find("div.grid-content-container"),f=c.get(0).columns,p=c.get(0).store,o=n.find("div.page-btn-container");var j=Math.ceil(p.data.length/d);i.totalPage=j;var h=m.find("table.grid-content-bg tbody");var b="";for(var l=0;l<d;l++){b+='<tr class="grid-content-tr grid-content-tr-'+l+'" >';for(var g=0;g<f.length;g++){var e=f[g];b+='<td class="grid-content-td grid-content-td-'+g+" grid-content-td-"+e.dataIndex+'" name="'+e.dataIndex+'">';b+='<span class="content"></span>';b+="</td>"}b+="</tr>"}h.empty().append(a(b));if(j>1){n.removeClass("hidden")}else{n.addClass("hidden")}m.css("height","auto");b="";if(j>i.maxPageBtnNum){for(var l=0;l<j;l++){if(l<i.maxPageBtnNum){b+='<div class="button-container inline">'}else{b+='<div class="button-container hidden">'}b+='<button type="button" class="button-button page-btn" page-num="'+l+'">';b+='<span class="text">'+(1+l)+"</span>";b+="</button>";b+="</div>"}}else{for(var l=0;l<j;l++){b+='<div class="button-container inline">';b+='<button type="button" class="button-button page-btn" page-num="'+l+'">';b+='<span class="text">'+(1+l)+"</span>";b+="</button>";b+="</div>"}}o.empty().append(a(b));if(k>=j){i.currentPage=0}n.paging("goToPage",i.currentPage);return n},loadPage:function(e,f){var e=e||this,g=e.get(0),c=g.grid,d=isNaN(f[1])?g.currentPage:f[1],b=parseInt(g.currentItemNumPerPage,10);c.grid("load",c.get(0).store.data,d*b,b);g.currentPage=parseInt(d,10);return e},goToPage:function(g,h){var g=g||this,e=g.get(0).grid,c=e.store,f=h[1],d=g.get(0).totalPage;g.find("div.button-container").removeClass("current");g.find("button.page-btn[page-num="+f+"]").closest("div.button-container").addClass("current");g.paging("loadPage",f);var i=g.find("button.prevoius-page-btn").closest("div.button-container"),b=g.find("button.next-page-btn").closest("div.button-container");if(f==0){i.addClass("disabled");b.removeClass("disabled")}else{if(f==d-1){i.removeClass("disabled");b.addClass("disabled")}else{i.removeClass("disabled");b.removeClass("disabled")}}return g},goPrev:function(e){var e=e||this,f=e.get(0),c=f.grid,b=c.store,d=f.currentPage;d--;if(d<0){return e}else{e.paging("goToPage",d)}return e},goNext:function(d){var d=d||this,e=d.get(0),c=e.currentPage,b=e.totalPage;c++;if(c>=b){return d}else{d.paging("goToPage",c)}return d}})})(jQuery);
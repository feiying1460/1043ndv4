(function(a){a.su.Widget("radio",{defaults:{name:null,items:[],cls:"",fieldLabel:null,boxLablePos:"right",tips:"",columns:1},create:function(e,c){var d=this;d.each(function(w,p){var n=a(this);var s=c.id||this.id||e.id,t=c.value||n.val()||this.value||e.value,C=c.name||this.name||e.name;n.addClass("hidden");a.extend(this,e,c);n.attr({value:t,id:s,name:C}).val(t).addClass("radio");var q='<div class="container widget-container radio-group-container '+this.cls+'">';if(this.fieldLabel!==null){q+='<div class="widget-fieldlabel-wrap '+this.labelCls+'">';q+='<label class="widget-fieldlabel radio-group-fieldlabel">'+this.fieldLabel+"</label>";if(this.fieldLabel!==""){q+='<span class="widget-separator">'+this.separator+"</span>"}q+="</div>"}var r=this.items,y=r.length,u=Math.ceil(y/this.columns),A=0,j=false,B=false;q+='<div class="radio-group-wrap">';q+='<ul class="radio-group-list-wrap">';var h=function(I,F,E,H,i,G,D){q+='<li class="radio-list">';q+='<div class="widget-wrap">';q+='<label class="radio-label '+x+'" for="'+E+'">';q+='<input style="display:none;" class="radio-radio" type="radio" name="'+I+'" value="'+F+'" id="'+E+'" '+l+" />";if(G=="left"){q+='<span class="text '+i+'">'+z.boxlabel+"</span>";q+='<span class="icon"></span>'}else{q+='<span class="icon"></span>';q+='<span class="text '+i+'">'+z.boxlabel+"</span>"}q+="</label>";q+="</div>";if(D){q+='<div class="radio-content-wrap inline"></div>'}else{q+='<div class="radio-content-wrap"></div>'}q+="</li>"};for(var k=0;k<r.length;k++){var z=r[k],f=z.name||C||"",m=z.inputvalue||t||"",g=z.id||a.su.randomId("radio");boxlabelCls=z.boxlabelCls||"";j=z.content?true:j;B=z.contentInline?true:B;var l="",x="";if(z.checked==="checked"||z.checked===true){l='checked="checked"';x="checked";n.val(m)}if(A<u){h(f,m,g,z.boxlabel,boxlabelCls,this.boxLablePos,B)}else{q+="</ul>";q+='<ul class="radio-group-list-wrap">';h(f,m,g,z.boxlabel,boxlabelCls,this.boxLablePos,B);A=0}A++}q+="</ul>";q+="</div>";if(this.tips!==""){q+='<span class="widget-tips">'+this.tips+"</span>"}q+="</div>";var o=a(q);n.addClass("radio-value").attr("disabled",true).replaceWith(o);o.prepend(n);if(j){for(var k=0;k<r.length;k++){var z=r[k];if(z.content){var v=a(z.content).addClass((z.contentCls||""));o.find("li.radio-list").eq(k).find("div.radio-content-wrap").append(v)}else{continue}}}});var b=d.radio("getContainer");b.delegate("label.radio-label","click",function(j){j.stopPropagation();j.preventDefault();var h=a(this),f=h.closest("li.radio-list"),g=h.find("input.radio-radio"),i=d.val(),k=g.val();if(f.hasClass("disabled")){return}else{d.radio("setValue",k)}d.trigger("ev_click",[k])});return d},getValue:function(b){var b=b||this;return b.val()},setValue:function(f,d){var f=f.filter(".radio-value")||this,e=f.val(),g=d[1],b=f.radio("getContainer");var c=b.find('input.radio-radio[value="'+g+'"][name='+f.get(0).name+"]");c.prop("checked",true);b.find("label.radio-label:has(input[name="+f.get(0).name+"])").removeClass("checked");c.closest("label.radio-label").addClass("checked");f.val(g);if(e.toString()!==g.toString()){f.trigger("ev_change",[e,g])}return f},disableItem:function(d,c){var d=d||this,e=d.get(0),b=d.radio("getContainer"),g=b.find("input.radio-radio"),c=c[1];if(a.type(c)==="string"||a.type(c)==="number"){c=[c]}var f=(function(){var i={};for(var h=0;h<c.length;h++){i[c[h]]=true}return i})();g.each(function(j,k){var h=a(k);if(h.val() in f){h.closest("li.radio-list").addClass("disabled");k.disabled=true}});return d},enableItem:function(d,c){var d=d||this,e=d.get(0),b=d.radio("getContainer"),g=b.find("input.radio-radio"),c=c[1];if(a.type(c)==="string"||a.type(c)==="number"){c=[c]}var f=(function(){var i={};for(var h=0;h<c.length;h++){i[c[h]]=true}return i})();g.each(function(j,k){var h=a(k);if(h.val() in f){h.closest("li.radio-list").removeClass("disabled");k.disabled=false}});return d},hideItem:function(d,c){var d=d||this,e=d.get(0),b=d.combobox("getContainer"),g=b.find("input.radio-radio"),c=c[1];if(a.type(c)==="string"||a.type(c)==="number"){c=[c]}var f=(function(){var i={};for(var h=0;h<c.length;h++){i[c[h]]=true}return i})();g.each(function(j,k){var h=a(k);if(h.val() in f){h.closest("li.radio-list").hide()}})},showItem:function(d,c){var d=d||this,e=d.get(0),b=d.combobox("getContainer"),g=b.find("input.radio-radio"),c=c[1];if(a.type(c)==="string"||a.type(c)==="number"){c=[c]}var f=(function(){var i={};for(var h=0;h<c.length;h++){i[c[h]]=true}return i})();g.each(function(j,k){var h=a(k);if(h.val() in f){h.closest("li.radio-list").show()}})},disableAll:function(c){var c=c||this,d=c.get(0),b=c.radio("getContainer"),e=b.find("input.radio-radio");e.each(function(f,g){a(g).closest("li.radio-list").addClass("disabled");g.disabled=true})},enableAll:function(c){var c=c||this,d=c.get(0),b=c.radio("getContainer"),e=b.find("input.radio-radio");e.each(function(f,g){a(g).closest("li.radio-list").removeClass("disabled");g.disabled=false});return c}})})(jQuery);
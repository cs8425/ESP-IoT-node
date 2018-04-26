'use strict';

function pand2(i) {
	var o = i
	if (i < 10) {o = "0" + o}
	return o;
}
function toTimeStr(t) {
	var ss = pand2(t % 60)
	t = Math.floor(t / 60)
	var mm = pand2(t % 60)
	t = Math.floor(t / 60)
	var hh = pand2(t % 60)
	return hh + ':' + mm + ':' + ss
}

/*$(document).ready(function(e) {

})*/
var url = 'http://192.168.1.116'


function mkStatus() {
	var e = $('#status')
	var out = {}
	out.e = e

	var p = ["temp", "hum", "pin", "on", "off", "heap"]
	p.forEach(function(v,k){
		//console.log(k,v)
		out[v] = e.find('div:nth-child(' + (k+1)+ ') > div:nth-child(2)')
	})

	return out
}
var stEle = mkStatus()
var st = null

function getStatus(cb) {
	$.ajax({
	url: url + '/status',
	type: 'GET',
	crossDomain: true,
	success: function(obj){
		//console.log('/status', obj)
		st = obj
		if(cb) cb(obj)
		updateStatusEle(stEle, obj)
	}})
}

function updateStatusEle(e, o) {
	e.pin.text( (o.pin) ? 'ON' : 'OFF' )
	e.heap.text(o.heap)
	if(o.md) {
		e.on.text( o.md[0] )
		e.off.text( o.md[1] )
	}
	if(o.sen) {
		e.temp.text( o.sen[0] / 100.0 )
		e.hum.text( o.sen[1] / 100.0 )
	}
}


var schedule = null
function getSchedule(cb) {
	$.ajax({
	url: url + '/sch/ls',
	type: 'GET',
	crossDomain: true,
	error: console.log,
	success: function(obj){
		console.log('/sch/ls', obj)
		schedule = obj
		if(cb) cb(obj)
		updateSchEle(obj)
	}})
}
function updateSchEle(o) {
	var def = $('#sch-def')
	var cus = $('#sch-cus')

	// update default output rule
	for(var i=0; i<7; i++){
		var e = def.find('div > div:nth-child(' + (i+2) + ')')
		$(e[1]).text(o.def[i][0])
		$(e[2]).text(o.def[i][1])
	}

	// update custom output rule
	var tmpl = $(cus.find('> div:nth-child(1)')).clone()
	cus.html('')
	tmpl.appendTo(cus)
	var wmap = [7,1,2,3,4,5,6]
	for(var i=0; i<o.sch.length; i++){
		var sch = o.sch[i]
		var e = tmpl.clone()
		e.attr('mid', i)
		e.find('> div:nth-child(1)').text('')
		e.find('> div:nth-child(2)').text('w' + wmap[sch[0]]) // week
		e.find('> div:nth-child(3)').text(toTimeStr(sch[1])) // start
		e.find('> div:nth-child(4)').text(toTimeStr(sch[2])) // end
		e.find('> div:nth-child(5)').text(sch[3]) // on
		e.find('> div:nth-child(6)').text(sch[4]) // off
		e.appendTo(cus)
	}

	//disable all highlight
	def.find('div > div').removeClass('en')
	cus.find('> div').removeClass('en')

	var mid = o.mid
	if(mid < 0) { // default rule
		var wid = -mid - 10
		var divs = def.find('div > div:nth-child(' + (wid+1) + ')')
		divs.addClass('en')
	} else { // custom rule
		var list = cus.find('> div')
		for(var i=0; i<list.length; i++) {
			var e = $(list[i])
			if(e.attr('mid') == mid) {
				e.addClass('en')
			}
		}
	}
}

function init(){

	// navbar
	$('.nav > div.btn').on('click', function(e){
		var id = $(this).text().toLowerCase()
		//console.log(e, this, id)
		$('.tabs > div.block').css('display', 'none')
		$('#' + id).css('display', 'block')
	})

	// bind default setting handler
	$('#sch-def > div:nth-child(1) > div').on('click', function(e){
		console.log(e, this)
		// setDef(type)
	})
}
init()

function poll() {
	getStatus();
	var t = setTimeout(poll, 1000)
}
poll()
getSchedule()


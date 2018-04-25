'use strict';

function toDateTimeString(t) {
	return new Date(t).toLocaleString()
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
		console.log('/status', obj)
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
	success: function(data){
		console.log('/sch/ls', data)
		schedule = data
		if(cb) cb(data)
	}})
}

function poll() {
	getStatus();
	var t = setTimeout(poll, 1000)
}
poll()



function ts_to_time( ts )
{
    // Create a new JavaScript Date object based on the timestamp
    // multiplied by 1000 so that the argument is in milliseconds, not seconds.
    var date = new Date(ts*1000);
    return date.toUTCString();
}

function pad(num, size){ return ('000000000' + num).substr(-size); }

function display_c(){
    var refresh=1000; // Refresh rate in milli seconds
    mytime=setTimeout('display_ct()',refresh)
}

function display_ct()
{
    var strcount
    var x = new Date()
    var x1 = x.getUTCFullYear() + "-" + pad( x.getUTCMonth() + 1, 2 ) + "-" + pad( x.getUTCDate(), 2 );
    x1 = x1 + " " +  pad( x.getUTCHours(), 2 ) + ":" +  pad( x.getUTCMinutes(), 2 ) + ":" +  pad( x.getUTCSeconds(), 2 );
    document.getElementById('utc_clock').innerHTML = x1;
    tt=display_c();
}

$( document ).ready(function() {
    $(".table-sorted").tablesorter( { sortList: [[0,0]] } );
    $(".table-sorted-asc").tablesorter( { sortList: [[0,0]] } );
    $(".table-sorted-desc").tablesorter( { sortList: [[0,1]] } );
    display_ct();
    $(".datetimepicker").datetimepicker( { pick12HourFormat: false, sideBySide: true, format: 'YYYY-MM-DD HH:mm:ss', useSeconds: true } );
});
// Configuration of the API endpoint 
const config = {
   // AWS API Gateway URL to be inserted here
    api_url: 'https://if2p7qktyf.execute-api.us-east-1.amazonaws.com/dev' 
  };

/* Global Data */

 // Data for system chart
 var x_values_system = [];
 var y_values_system = [];
 // Data for alarm chart
 var x_values_alarm = [];
 var y_values_alarm = [];

/* Statistics */

// Average measured magnetic field in case of alarm
var avg_alarm_field = 0;
// Number of days with triggered alarms
var days_with_alarms = 0;

// Average system activation time
var avg_act_time = 0;

var alarm_values = [];

function dataLoading(parsed_data){

  // Extracting body_alarm section of the response
  const alarm_data = parsed_data.body_alarm;

  // Extracting body_system section of the response
  const system_data = parsed_data.body_system;

  var dict = {};

  for (let i = 0; i < alarm_data.length; i++) {

    var d = new Date(alarm_data[i]["sample_time"]).toLocaleString();
    // Extracting only date as a key for dict
    var current_date = d.substring(0,10)

    if (typeof dict[current_date] === "undefined") {
      dict[current_date] = 1;
    }
    
  }

  days_with_alarms = Object.keys(dict).length;

  // Select last 6 values if there are more than 6
  var start = 0;
  if (alarm_data.length > 6) {
    start = alarm_data.length - 6;
  }

  // Alarm data processing
  for (let i = start; i < alarm_data.length; i++) {

    const sample_time = alarm_data[i]["sample_time"];
    // Convert sample time to date and to Locale format
    var datetime = new Date(sample_time).toLocaleString();

    var device_id = alarm_data[i]["device_id"];
    var alarm = alarm_data[i]["device_data"]["alarm"];
    if (alarm) {
      var value = ("ID: ").concat(device_id).concat(" Date: ").concat(datetime); 
      // Add value to x_values array
      x_values_alarm.push(value); 

      value = alarm_data[i]["device_data"]["m_field"];
      // Add value to y_valued array
      y_values_alarm.push(value);
    }

  }

  // Select last 12 values if there are more than 12 (6 pairs on/off)
  var start = 0;
  if (system_data.length > 12) {
    start = system_data.length - 12;
  }

  // System data processing
  for (let i = start; i < system_data.length; i++) {

    var device_id = system_data[i]["device_id"];
    // Assumption: system data come from only from device with ID 1 (with activation/deactivation button)
    if (device_id != 1) break

    const sample_time = system_data[i]["sample_time"];
    // Convert sample time to date and to Locale format
    var datetime = new Date(sample_time).toLocaleString();
    var system = system_data[i]["device_data"]["system"];

    if (i < system_data.length - 1 && system == 1) {

      var device_id_next = system_data[i + 1]["device_id"];
      if (device_id_next != 1) break;

      const sample_time_next  = system_data[i + 1]["sample_time"];
      var datetime_next = new Date(sample_time_next).toLocaleString();
      var system_next = system_data[i + 1]["device_data"]["system"];

      if (system_next == 0) {
        var value = [(" On: ").concat(datetime), ("Off: ").concat(datetime_next)];
        
        // Add value to x_values array
        x_values_system.push(value); 

        var d1 = new Date(sample_time);
        var d2 = new Date(sample_time_next);

        // Conversion from ms to min
        value =  (((d2 - d1)/1000)/60).toFixed(3);
        // Add value to y_valued array
        y_values_system.push(value);

      }
    }

  }
}

/* Alarm statistics */
async function getAlarmStatistics() {

  // Reset statistics
  avg_alarm_field = 0;
  var total_field = 0;

  for (let i = 0; i < y_values_alarm.length; i++) total_field += parseFloat(y_values_alarm[i]);

  avg_alarm_field = parseFloat(total_field/y_values_alarm.length);
}


 /* System statistics */
async function getSystemStatistics() {

  // Reset statistics 
  avg_act_time = 0;
  var total_act_time = 0;

  for (let i = 0; i < y_values_system.length; i++) {
    total_act_time += parseFloat(y_values_system[i]);
  }

  avg_act_time = parseFloat(total_act_time/y_values_system.length);

}

    
function plot(id, x_values, y_values) {
  var color = "black";

  if (id === "alarmchart") {
    color = "red";
  }
  else if (id === "systemchart") {
    color = "blue";
  }

  const ctx = document.getElementById(id).getContext('2d');
  new Chart(ctx, {
    type: "bar",
    data: {
      labels: x_values,
      datasets: [{
        backgroundColor: color,
        data: y_values
      }]
    },
    options: {
      legend: {display: false},
      title: {display: false},
      scales: {
        yAxes: [{
            ticks: { beginAtZero: true }
        }]
      }
    }
  });
}

async function display() {
  await callAPI()

  plot("alarmchart", x_values_alarm, y_values_alarm);
  plot("systemchart", x_values_system, y_values_system);

  getAlarmStatistics();
  document.getElementById("avg-alarm-field").innerHTML = Number((avg_alarm_field).toFixed(1));  
  document.getElementById("days-with-alarms").innerHTML = Number((days_with_alarms).toFixed(1));

  // Check every 2 seconds
  var intervalId = setInterval(checkAlarm, 2000);
  checkAlarm();

  getSystemStatistics();
  document.getElementById("avg-activation").innerHTML = Number((avg_act_time).toFixed(1));
}


function checkAlarm() {

  if (!Array.isArray(y_values_alarm) || !y_values_alarm.length) {
    document.getElementById("alarm-info").innerHTML = "No recent intrusion detected.";
  }

  else {
    document.getElementById("alarm-info").innerHTML = "ALERT: recent intrusion detected!";
    // Display alert only once to avoid continuous alerting and so an unusable dashboard
    var alerted = localStorage.getItem('alerted') || '';
    if (alerted != 'yes') {
     alert("ALERT: recent intrusion detected!");
     localStorage.setItem('alerted','yes');
    }
  }

}



async function callAPI() {

  // Instantiate a headers object
  var headers = new Headers();
  
  // Set the options of the request
  var requestOptions = {
      method: 'GET',
      headers: headers 
  };
  
  // Fetch content from API 
  const response = await fetch(config.api_url, requestOptions);
  const parsed_data = await response.json();
  dataLoading(parsed_data);

}

async function systemHandler() {

  var button_text = document.getElementById("system_button").innerHTML;

  // Instantiate a headers object
  var headers = new Headers();

  // Adding content type header to object
  headers.append("Content-Type", "application/json");

  // Using built-in JSON utility to turn object to string 
  if (button_text == "ACTIVATE") var raw = JSON.stringify({"system": "1"});
  else var raw = JSON.stringify({"system": "0"});
  
  // Set the options of the request
  var requestOptions = {
    method: 'POST',
    headers: headers,
    body: raw,
    redirect: 'follow'
  };

  // Fetch content from API 
  await fetch(config.api_url, requestOptions);

  if (button_text == "ACTIVATE") document.getElementById("system_button").innerHTML = "DEACTIVATE"
  else document.getElementById("system_button").innerHTML = "ACTIVATE"

}

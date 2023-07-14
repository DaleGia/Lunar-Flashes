const imageTopic = "/image"
const getExposureTopic = "/exposureget"
const getGainTopic = "/gainget"
const getFrameRateTopic = "/framerateget"
const getPreviewFrameRateTopic = "/previewframerateget"
const getBitDepthTopic = "/bitdepthget"
const getAllTopics = "/getallvalues"

const setExposureTopic = "/exposureset"
const setGainTopic = "/gainset"
const setFrameRateTopic = "/framerateset"
const setPreviewFrameRateTopic = "/previewframerateset"
const setBitDepthTopic = "/bitdepthset"
const logTopic = "/LOG"

const histogramTopic = "/histogram";
const maxTopic = "/max";
const minTopic = "/min";
const averageTopic = "/average";


const frameReceivedTopic = "/framereceived";
const frameSavedTopic = "/framesaved";
const invalidFrameTopic = "/invalidframe";
const frameSavedDataTopic = "/framesavedatareceived";

var client;

var logCount = 0;

const ctx = document.getElementById('histogramchart').getContext('2d');

var chart = new Chart(
    ctx, 
    {
        type: 'line',
        data: 
        {
            labels: [],
            datasets: [{data: [], label: "histogram"}]
        },
        options: 
        {
            animation: false,
            indexAxis: 'x',
            scales: 
            {
              x: 
              {
                beginAtZero: true
              },
              y:
              {
                beginAtZero: true
              }

            }
        }
    });

function addData(chart, label, data) 
{
    var count = 0
    chart.data.labels.push(label);
    data.forEach((value) => {
        chart.data.labels.push(count);
        count++;
        chart.data.datasets[0].data.push(value)
    })

    chart.update();
}

function removeData(chart) 
{
    chart.data.labels = [];
    chart.data.datasets[0].data = [];
    chart.update();
}
    
function start()
{
    // Connect to the MQTT broker
    attemptMQTTConnection();
    addListeners();
}

window.addEventListener('load', function () 
{
    start();
})

function attemptMQTTConnection()
{
    console.log("Attempting MQTT Connection...")
    client = new Paho.Client("localhost", 8080, new Date().toLocaleString());

    // Callback function to execute when a message is received
    client.onMessageArrived = onmessage;
    client.onConnectionLost = function (responseObject) 
    {
        console.log("Connection Lost: "+responseObject.errorMessage);
        attemptMQTTConnection();
    }
    client.onFailure = function()
    {
        console.log("MQTT Connect failure detected... attempting reconnection")
        attemptMQTTConnection();
    };
    client.connect(
    {
        onSuccess: MQTTConnectionSuccess, 
        timeout: 2
    });
}
function MQTTConnectionSuccess()
{
    console.log("MQTT Connection success")

    // Subscribe to the "/image" topic
    client.subscribe(imageTopic);
    client.subscribe(getGainTopic);
    client.subscribe(getExposureTopic);
    client.subscribe(getFrameRateTopic);
    client.subscribe(getPreviewFrameRateTopic);
    client.subscribe(getBitDepthTopic);
    client.subscribe(logTopic);
    client.subscribe(histogramTopic);
    client.subscribe(maxTopic);
    client.subscribe(minTopic);
    client.subscribe(averageTopic);
    client.subscribe(frameReceivedTopic);
    client.subscribe(frameSavedTopic);
    client.subscribe(invalidFrameTopic);
    client.subscribe(frameSavedDataTopic);
    client.publish(getAllTopics, "1");
}
function onmessage(message) 
{
    try 
    {
        if (message.destinationName == imageTopic) 
        {
            // Change the image source to the payload of the message
            var image = document.getElementById("myImage");
            image.src = "data:image/jpeg;base64," + message.payloadString;
        }
        else if (message.destinationName == getGainTopic) 
        {
            console.log("Got gain " + message.payloadString)
            var gain = document.getElementById("gain");
            gain.value = message.payloadString
        }
        else if (message.destinationName == getExposureTopic) 
        {
            console.log("Got exposure " + message.payloadString)
            var exposure = document.getElementById("exposure");  
            exposure.value = message.payloadString      
        }
        else if (message.destinationName == getFrameRateTopic) 
        {
            console.log("Got framerate " + message.payloadString)
            var framerate = document.getElementById("framerate");
            framerate.value = message.payloadString   
        }
        else if (message.destinationName == getPreviewFrameRateTopic) 
        {
            console.log("Got preview framerate " + message.payloadString)
            var previewframerate = document.getElementById("previewframerate");
            previewframerate.value = message.payloadString   
        }
        else if (message.destinationName == getBitDepthTopic) 
        {
            console.log("Got bitdepth " + message.payloadString)
            var bitdepth = message.payloadString;
            var radio = document.getElementById(bitdepth);
            radio.checked = true;
        }
        else if (message.destinationName == maxTopic) 
        {
            var max = message.payloadString;
            document.getElementById("max").textContent = "max: " + max;
        }
        else if (message.destinationName == minTopic) 
        {
            var min = message.payloadString;
            document.getElementById("min").textContent = "min: " + min;

        }
        else if (message.destinationName == averageTopic) 
        {
            var average = message.payloadString;
            document.getElementById("average").textContent = "average: " + average;
        }
        else if (message.destinationName == frameReceivedTopic) 
        {
            document.getElementById("framereceived").textContent = 
            "frames received: " + message.payloadString;
        }
        else if (message.destinationName == frameSavedTopic) 
        {
            document.getElementById("framesaved").textContent = 
            "frames saved: " + message.payloadString;
        }
        else if (message.destinationName == invalidFrameTopic) 
        {
            document.getElementById("invalidframe").textContent = 
            "total invalid frames: " + message.payloadString;
        }
        else if (message.destinationName == frameSavedDataTopic) 
        {
            document.getElementById("framesavedatareceived").textContent = 
            "saved frames data rate (MB) " + message.payloadString;
        }
        else if (message.destinationName == histogramTopic) 
        {
            var histogram = JSON.parse(message.payloadString);
            removeData(chart);
            addData(chart, "histogram", histogram);
        }
        else if(message.destinationName == logTopic)
        {
            
            let log = document.getElementById("log");

            let newLog = document.createElement('div')
            newLog.classList.add("logEntry");
            newLog.id = 'log-' + logCount.toString();
            logCount++;
            newLog.innerHTML = message.payloadString;
            newLog.classList = "text"
            log.insertBefore(newLog, log.childNodes[0]);
        }
    }
    catch(err)
    {
        console.log(err)
    }
};
function setGain(gain_db)
{   
    console.log("Setting gain to " + gain_db)
    client.publish(setGainTopic, gain_db)
}

function setExposure(time_us)
{   
    client.publish(setExposureTopic, time_us)
}


function setFramerate(fps)
{   
    client.publish(setFrameRateTopic, fps)
}

function setPreviewFramerate(fps)
{   
    client.publish(setPreviewFrameRateTopic, fps);
}

function setBitdepth(bitdepth)
{   
    client.publish(setBitDepthTopic, bitdepth)
}

function addListeners()
{
    let bitdepth = document.getElementById("bitdepth");
    let framerate = document.getElementById("framerate");
    let previewframerate = document.getElementById("previewframerate");
    let gain = document.getElementById("gain");
    let exposure = document.getElementById("exposure");
    let startButton = document.getElementById("start");
    let stopButton = document.getElementById("stop");
    let saveButton = document.getElementById("save");
    let loadButton = document.getElementById("load");
    let connectButton = document.getElementById("connect");
    let radioButtons = document.querySelectorAll('input[type="radio"]');

    let bitdepthValue = document.getElementById("bitdepthValue");
    let framerateValue = document.getElementById("framerateValue");
    let previewframerateValue = document.getElementById("previewframerateValue");
    let gainValue = document.getElementById("gainValue");
    let exposureValue = document.getElementById("exposureValue");

    // Add event listener to each radio button
    radioButtons.forEach(function(radioButton) 
    {
        radioButton.addEventListener('click', function() 
        {
            setBitdepth(radioButton.value);
        });
    });

    connect.addEventListener("click", function() 
    {
        client.publish("/connect", "1")
        console.log("sent connect message")
    });


    startButton.addEventListener("click", function() 
    {
        client.publish("/start", "1")
    });

    stopButton.addEventListener("click", function() 
    {
        client.publish("/stop", "1")
    });

    saveButton.addEventListener("click", function() 
    {
        client.publish("/save", "1")
    });

    loadButton.addEventListener("click", function() 
    {
        client.publish("/load", "1")
    });

    framerate.addEventListener("change", function() 
    {
        setFramerate(framerate.value);
    }); 

    previewframerate.addEventListener("change", function() 
    {
        setPreviewFramerate(previewframerate.value);
    });

    gain.addEventListener("change", function() 
    {
        setGain(gain.value);
    });

    exposure.addEventListener("change", function() 
    {
        setExposure(exposure.value);
    });
}
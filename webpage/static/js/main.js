const url = "0.0.0.0"

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

const captureSetTopic = "/captureset"
const captureGetTopic = "/captureget"
const saveSetTopic = "/saveset"
const saveGetTopic = "/saveget"

var client;
var recordingStatus;
var captureStatus
var logCount = 0;

const ctx = document.getElementById('histogramchart').getContext('2d');
var chart;

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
    chart = new Chart(
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
    
    start();
})

function attemptMQTTConnection()
{
    console.log("Attempting MQTT Connection...")
    client = new Paho.Client(url, 8080, new Date().toLocaleString());

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
    client.subscribe(saveGetTopic);
    client.subscribe(captureGetTopic);

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
            "Frames Received (FPS): " + message.payloadString;
        }
        else if (message.destinationName == frameSavedTopic) 
        {
            document.getElementById("framesaved").textContent = 
            "Frames Saved (FPS): " + message.payloadString;
        }
        else if (message.destinationName == invalidFrameTopic) 
        {
            document.getElementById("invalidframe").textContent = 
            "Total Invalid Frames: " + message.payloadString;
        }
        else if (message.destinationName == frameSavedDataTopic) 
        {
            document.getElementById("framesavedatareceived").textContent = 
            "Received Frames Data Rate (MBps) " + message.payloadString;
        }
        else if (message.destinationName == histogramTopic) 
        {
            var histogram = JSON.parse(message.payloadString);
            removeData(chart);
            addData(chart, "histogram", histogram);
        }
        else if(message.destinationName == saveGetTopic)
        {
            console.log("save status: " + message.payloadString)
            if(message.payloadString == "0")
            {
                document.getElementById("start-recording").textContent =
                    "Start Recording" 
                document.getElementById("start-recording").classList = 
                    "button green"
                recordingStatus = false;
            }
            else
            {
                document.getElementById("start-recording").textContent =
                "Stop Recording" 
                document.getElementById("start-recording").classList = 
                    "button red"
                recordingStatus = true;
            }
        }
        else if(message.destinationName == captureGetTopic)
        {
            if(message.payloadString == "0")
            {
                document.getElementById("start-capture").textContent =
                    "Start Capture" 
                document.getElementById("start-capture").classList = 
                    "button green"
                captureStatus = false;
            }
            else
            {
                document.getElementById("start-capture").textContent =
                "Stop Capture" 
                document.getElementById("start-capture").classList = 
                    "button red"
                captureStatus = true;
            }
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
    let startButton = document.getElementById("start-capture");
    let recordButton = document.getElementById("start-recording");
    let stopButton = document.getElementById("stop-capture");
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
        if(captureStatus == true)
        {
            client.publish(captureSetTopic, "0")
            console.log("Sent stop capture signal")
        }
        else
        {
            client.publish(captureSetTopic, "1")
            console.log("Sent start capture signal")

        }
    });

    recordButton.addEventListener("click", function() 
    {
        if(recordingStatus == true)
        {
            client.publish(saveSetTopic, "0")
            console.log("Sent stop recording signal")
        }
        else
        {
            client.publish(saveSetTopic, "1")
            console.log("Sent start recording signal")

        }
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
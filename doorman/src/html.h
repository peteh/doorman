#include<Arduino.h>
const PROGMEM char PAGE_SETTINGS[] = "<!DOCTYPE html>\n<html>\n\n<head>\n    <title>Doorman Configuration</title>\n        <style>\n        body {\n            font-family: Arial, sans-serif;\n            margin: 20px;\n        }\n\n        h1 {\n            text-align: center;\n        }\n\n        form {\n            max-width: 500px;\n            margin: 0 auto;\n        }\n\n        table {\n            width: 100%;\n            border-collapse: collapse;\n        }\n\n        th,\n        td {\n            padding: 10px;\n            text-align: left;\n        }\n\n        th {\n            background-color: #f2f2f2;\n        }\n\n        input[type=\"text\"] {\n            width: 100%;\n            padding: 8px;\n            border: 1px solid #ccc;\n            border-radius: 4px;\n            box-sizing: border-box;\n        }\n\n        input[type=\"submit\"] {\n            background-color: #4CAF50;\n            color: #fff;\n            padding: 10px 20px;\n            border: none;\n            border-radius: 4px;\n            cursor: pointer;\n            float: right;\n        }\n\n        input[type=\"submit\"]:hover {\n            background-color: #45a049;\n        }\n\n        #feedback {\n            margin-top: 10px;\n            color: #f00;\n            text-align: center; /* Center the error message */\n            display: block; /* Ensure the div behaves as a block-level element */\n            width: 100%; /* Span the entire width of its parent container */\n            clear: both; /* Clear the float after the submit button */\n        }\n        #menu {\n            margin-top: 10px;\n            color: #00f;\n            text-align: center; /* Center the error message */\n            display: block; /* Ensure the div behaves as a block-level element */\n            width: 100%; /* Span the entire width of its parent container */\n            clear: both; /* Clear the float after the submit button */\n        }\n        #version {\n            margin-top: 10px;\n            color: #000;\n            text-align: center;\n            display: block; /* Ensure the div behaves as a block-level element */\n            width: 100%; /* Span the entire width of its parent container */\n            clear: both; /* Clear the float after the submit button */\n        }\n    </style>\n\n</head>\n\n<body>\n    <h1>Doorman Configuration Page: Wifi</h1>\n    <div id=\"menu\">\n        <a href=\"codes\">Codes</a> <a href=\"settings\">Settings</a>\n</div>\n    <form id=\"configForm\">\n        <table>\n            <tr>\n                <th>Setting</th>\n                <th>Value</th>\n            </tr>\n            <tr>\n                <td>Wifi SSID</td>\n                <td><input type=\"text\" name=\"wifiSsid\" maxlength=\"150\" required></td>\n            </tr>\n            <tr>\n                <td>Wifi Password (not shown)</td>\n                <td><input type=\"text\" name=\"wifiPassword\" maxlength=\"150\"></td>\n            </tr>\n            <tr>\n                <td>MQTT Server</td>\n                <td><input type=\"text\" name=\"mqttServer\" maxlength=\"150\" required></td>\n            </tr>\n            <tr>\n                <td>MQTT Port</td>\n                <td><input type=\"text\" name=\"mqttPort\" maxlength=\"8\" required></td>\n            </tr>\n            <tr>\n                <td>MQTT User</td>\n                <td><input type=\"text\" name=\"mqttUser\"></td>\n            </tr>\n            <tr>\n                <td>MQTT Password</td>\n                <td><input type=\"text\" name=\"mqttPassword\" maxlength=\"8\"></td>\n            </tr>\n        </table>\n        <div id=\"feedback\"></div>\n        <div id=\"version\"></div>\n        <br>\n        <input type=\"submit\" value=\"Save Configuration\">\n        \n    </form>\n    <script>\n        // Function to fetch and populate the configuration data\n        function populateConfig() {\n            // Fetch the current configuration from the ESP8266\n            fetch('/getSettings', {\n                method: 'GET',\n            })\n                .then(response => response.json())\n                .then(data => {\n                    // Populate the input fields with the retrieved values\n                    document.getElementById(\"configForm\").elements[\"wifiSsid\"].value = data.wifiSsid;\n                    //document.getElementById(\"configForm\").elements[\"wifiPassword\"].value = data.wifiPassword;\n                    document.getElementById(\"configForm\").elements[\"mqttServer\"].value = data.mqttServer;\n                    document.getElementById(\"configForm\").elements[\"mqttPort\"].value = data.mqttPort;\n                    document.getElementById(\"configForm\").elements[\"mqttUser\"].value = data.mqttUser;\n                    document.getElementById(\"configForm\").elements[\"mqttPassword\"].value = data.mqttPassword;\n                    document.getElementById(\"version\").textContent = data.version;\n                    \n                })\n                .catch(error => {\n                    console.error('Error:', error);\n                    const feedbackDiv = document.getElementById(\"feedback\");\n                    feedbackDiv.textContent = \"An error occurred while fetching the configuration.\";\n                });\n        }\n\n        // Call the function to populate the configuration on page load\n        window.addEventListener('load', populateConfig);\n\n        // Handle form submission\n        document.getElementById(\"configForm\").onsubmit = function (event) {\n            event.preventDefault(); // Prevent form from submitting normally\n\n            // Get form data\n            const formData = new FormData(event.target);\n\n            // Convert form data to a JSON object\n            const configData = {};\n            formData.forEach((value, key) => {\n                configData[key] = value;\n            });\n\n\n            // Send the configuration data to the server (ESP8266)\n            fetch('/setSettings', {\n                method: 'POST',\n                headers: {\n                    'Content-Type': 'application/json'\n                },\n                body: JSON.stringify(configData)\n            })\n                .then(response => response.json())\n                .then(data => {\n                    // Display feedback to the user\n                    const feedbackDiv = document.getElementById(\"feedback\");\n                    feedbackDiv.textContent = data.message;\n                })\n                .catch(error => {\n                    console.error('Error:', error);\n                    const feedbackDiv = document.getElementById(\"feedback\");\n                    feedbackDiv.textContent = \"An error occurred while saving the configuration.\";\n                });\n        };\n        // Validate input fields to accept only hexadecimal numbers\n        /**\n        const hexInputFields = document.querySelectorAll('input[type=\"text\"]');\n        hexInputFields.forEach(field => {\n            field.addEventListener('input', () => {\n                const inputValue = field.value;\n                const isValidHex = /^[0-9a-fA-F]*$/.test(inputValue);\n                if (!isValidHex) {\n                    field.setCustomValidity(\"Please enter a valid hexadecimal number.\");\n                } else {\n                    field.setCustomValidity(\"\");\n                }\n            });\n        });\n        */\n    </script>\n</body>\n\n</html>\n";
const PROGMEM char PAGE_MAIN[] = "<!DOCTYPE html>\n<html>\n\n<head>\n    <title>Doorman Configuration</title>\n        <style>\n        body {\n            font-family: Arial, sans-serif;\n            margin: 20px;\n        }\n\n        h1 {\n            text-align: center;\n        }\n\n        form {\n            max-width: 500px;\n            margin: 0 auto;\n        }\n\n        table {\n            width: 100%;\n            border-collapse: collapse;\n        }\n\n        th,\n        td {\n            padding: 10px;\n            text-align: left;\n        }\n\n        th {\n            background-color: #f2f2f2;\n        }\n\n        input[type=\"text\"] {\n            width: 100%;\n            padding: 8px;\n            border: 1px solid #ccc;\n            border-radius: 4px;\n            box-sizing: border-box;\n        }\n\n        input[type=\"submit\"] {\n            background-color: #4CAF50;\n            color: #fff;\n            padding: 10px 20px;\n            border: none;\n            border-radius: 4px;\n            cursor: pointer;\n            float: right;\n        }\n\n        input[type=\"submit\"]:hover {\n            background-color: #45a049;\n        }\n\n        #feedback {\n            margin-top: 10px;\n            color: #f00;\n            text-align: center; /* Center the error message */\n            display: block; /* Ensure the div behaves as a block-level element */\n            width: 100%; /* Span the entire width of its parent container */\n            clear: both; /* Clear the float after the submit button */\n        }\n        #menu {\n            margin-top: 10px;\n            color: #00f;\n            text-align: center; /* Center the error message */\n            display: block; /* Ensure the div behaves as a block-level element */\n            width: 100%; /* Span the entire width of its parent container */\n            clear: both; /* Clear the float after the submit button */\n        }\n        #version {\n            margin-top: 10px;\n            color: #000;\n            text-align: center;\n            display: block; /* Ensure the div behaves as a block-level element */\n            width: 100%; /* Span the entire width of its parent container */\n            clear: both; /* Clear the float after the submit button */\n        }\n    </style>\n\n</head>\n\n<body>\n    <h1>Doorman Configuration Page</h1>\n    <div id=\"menu\">\n        <a href=\"codes\">Codes</a> <a href=\"settings\">Settings</a>\n</div>\n    <div id=\"version\"></div>\n\n    <script>\n        // Function to fetch and populate the configuration data\n        function populateConfig() {\n            // Fetch the current configuration from the ESP8266\n            fetch('/getCodes', {\n                method: 'GET',\n            })\n                .then(response => response.json())\n                .then(data => {\n                    document.getElementById(\"version\").textContent = data.version;\n                    \n                })\n                .catch(error => {\n                    console.error('Error:', error);\n                    const feedbackDiv = document.getElementById(\"feedback\");\n                    feedbackDiv.textContent = \"An error occurred while fetching the configuration.\";\n                });\n        }\n\n        // Call the function to populate the configuration on page load\n        window.addEventListener('load', populateConfig);\n    </script>\n</body>\n\n</html>\n";
const PROGMEM char PAGE_CODES[] = "<!DOCTYPE html>\n<html>\n\n<head>\n    <title>Doorman Configuration</title>\n        <style>\n        body {\n            font-family: Arial, sans-serif;\n            margin: 20px;\n        }\n\n        h1 {\n            text-align: center;\n        }\n\n        form {\n            max-width: 500px;\n            margin: 0 auto;\n        }\n\n        table {\n            width: 100%;\n            border-collapse: collapse;\n        }\n\n        th,\n        td {\n            padding: 10px;\n            text-align: left;\n        }\n\n        th {\n            background-color: #f2f2f2;\n        }\n\n        input[type=\"text\"] {\n            width: 100%;\n            padding: 8px;\n            border: 1px solid #ccc;\n            border-radius: 4px;\n            box-sizing: border-box;\n        }\n\n        input[type=\"submit\"] {\n            background-color: #4CAF50;\n            color: #fff;\n            padding: 10px 20px;\n            border: none;\n            border-radius: 4px;\n            cursor: pointer;\n            float: right;\n        }\n\n        input[type=\"submit\"]:hover {\n            background-color: #45a049;\n        }\n\n        #feedback {\n            margin-top: 10px;\n            color: #f00;\n            text-align: center; /* Center the error message */\n            display: block; /* Ensure the div behaves as a block-level element */\n            width: 100%; /* Span the entire width of its parent container */\n            clear: both; /* Clear the float after the submit button */\n        }\n        #menu {\n            margin-top: 10px;\n            color: #00f;\n            text-align: center; /* Center the error message */\n            display: block; /* Ensure the div behaves as a block-level element */\n            width: 100%; /* Span the entire width of its parent container */\n            clear: both; /* Clear the float after the submit button */\n        }\n        #version {\n            margin-top: 10px;\n            color: #000;\n            text-align: center;\n            display: block; /* Ensure the div behaves as a block-level element */\n            width: 100%; /* Span the entire width of its parent container */\n            clear: both; /* Clear the float after the submit button */\n        }\n    </style>\n\n</head>\n\n<body>\n    <h1>Doorman Configuration Page: Codes</h1>\n    <div id=\"menu\">\n        <a href=\"codes\">Codes</a> <a href=\"settings\">Settings</a>\n</div>\n    <form id=\"configForm\">\n        <table>\n            <tr>\n                <th>Setting</th>\n                <th>Value (Hexadecimal)</th>\n            </tr>\n            <tr>\n                <td>Apartment Door Bell Code</td>\n                <td><input type=\"text\" name=\"codeApartmentDoorBell\" maxlength=\"8\" required></td>\n            </tr>\n            <tr>\n                <td>Entry Door Bell Code</td>\n                <td><input type=\"text\" name=\"codeEntryDoorBell\" maxlength=\"8\" required></td>\n            </tr>\n            <tr>\n                <td>Handset Liftup Code</td>\n                <td><input type=\"text\" name=\"codeHandsetLiftup\" maxlength=\"8\" required></td>\n            </tr>\n            <tr>\n                <td>Door Opener Code</td>\n                <td><input type=\"text\" name=\"codeDoorOpener\" maxlength=\"8\" required></td>\n            </tr>\n            <tr>\n                <td>Apartment Door Pattern Detection Code<br />(When this pattern is detected, mqtt message will be sent)</td>\n                <td><input type=\"text\" name=\"codeApartmentPatternDetect\" required></td>\n            </tr>\n            <tr>\n                <td>Entry Door Pattern Detection<br />(When this pattern is detected, door opener will be sent)</td>\n                <td><input type=\"text\" name=\"codeEntryPatternDetect\" maxlength=\"8\" required></td>\n            </tr>\n            <tr>\n                <td>Entry Door Party Mode<br />(When this code is detected, the Door opener code is sent when party mode is enabled)</td>\n                <td><input type=\"text\" name=\"codePartyMode\" maxlength=\"8\" required></td>\n            </tr>\n        </table>\n        <div id=\"feedback\"></div>\n        <div id=\"version\"></div>\n        <br>\n        <input type=\"submit\" value=\"Save Configuration\">\n        \n    </form>\n    <script>\n        // Function to fetch and populate the configuration data\n        function populateConfig() {\n            // Fetch the current configuration from the ESP8266\n            fetch('/getCodes', {\n                method: 'GET',\n            })\n                .then(response => response.json())\n                .then(data => {\n                    // Populate the input fields with the retrieved values\n                    document.getElementById(\"configForm\").elements[\"codeApartmentDoorBell\"].value = data.codeApartmentDoorBell.toString(16).padStart(8, '0');\n                    document.getElementById(\"configForm\").elements[\"codeEntryDoorBell\"].value = data.codeEntryDoorBell.toString(16).padStart(8, '0');\n                    document.getElementById(\"configForm\").elements[\"codeHandsetLiftup\"].value = data.codeHandsetLiftup.toString(16).padStart(8, '0');\n                    document.getElementById(\"configForm\").elements[\"codeDoorOpener\"].value = data.codeDoorOpener.toString(16).padStart(8, '0');\n                    document.getElementById(\"configForm\").elements[\"codeApartmentPatternDetect\"].value = data.codeApartmentPatternDetect.toString(16).padStart(8, '0');\n                    document.getElementById(\"configForm\").elements[\"codeEntryPatternDetect\"].value = data.codeEntryPatternDetect.toString(16).padStart(8, '0');\n                    document.getElementById(\"configForm\").elements[\"codePartyMode\"].value = data.codePartyMode.toString(16).padStart(8, '0');\n                    document.getElementById(\"version\").textContent = data.version;\n                    \n                })\n                .catch(error => {\n                    console.error('Error:', error);\n                    const feedbackDiv = document.getElementById(\"feedback\");\n                    feedbackDiv.textContent = \"An error occurred while fetching the configuration.\";\n                });\n        }\n\n        // Call the function to populate the configuration on page load\n        window.addEventListener('load', populateConfig);\n\n        // Handle form submission\n        document.getElementById(\"configForm\").onsubmit = function (event) {\n            event.preventDefault(); // Prevent form from submitting normally\n\n            // Get form data\n            const formData = new FormData(event.target);\n\n            // Convert form data to a JSON object\n            const configData = {};\n            formData.forEach((value, key) => {\n                configData[key] = parseInt(value, 16);\n            });\n\n\n            // Send the configuration data to the server (ESP8266)\n            fetch('/setCodes', {\n                method: 'POST',\n                headers: {\n                    'Content-Type': 'application/json'\n                },\n                body: JSON.stringify(configData)\n            })\n                .then(response => response.json())\n                .then(data => {\n                    // Display feedback to the user\n                    const feedbackDiv = document.getElementById(\"feedback\");\n                    feedbackDiv.textContent = data.message;\n                })\n                .catch(error => {\n                    console.error('Error:', error);\n                    const feedbackDiv = document.getElementById(\"feedback\");\n                    feedbackDiv.textContent = \"An error occurred while saving the configuration.\";\n                });\n        };\n        // Validate input fields to accept only hexadecimal numbers\n        const hexInputFields = document.querySelectorAll('input[type=\"text\"]');\n        hexInputFields.forEach(field => {\n            field.addEventListener('input', () => {\n                const inputValue = field.value;\n                const isValidHex = /^[0-9a-fA-F]*$/.test(inputValue);\n                if (!isValidHex) {\n                    field.setCustomValidity(\"Please enter a valid hexadecimal number.\");\n                } else {\n                    field.setCustomValidity(\"\");\n                }\n            });\n        });\n    </script>\n</body>\n\n</html>\n";

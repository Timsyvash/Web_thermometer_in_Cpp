#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DHT.h>

const char *ssid = "EpsilonNET";
const char *password = "zaq123456";

#define PIN_SENSOR D1
#define SENSOR_TYPE DHT11

WiFiServer server(80);
DHT dht(PIN_SENSOR, SENSOR_TYPE);

void setup()
{
  Serial.begin(115200);
  delay(100);

  // Обов'язкова ініціалізація датчика
  dht.begin();

  Serial.print("\nConnecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    yield();

    attempts++;
    if (attempts > 30)
    {
      Serial.println("\nFailed to connect. Check SSID/Password or Signal!");
      break;
    }
  }

  Serial.println("\nWiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop()
{
  WiFiClient client = server.accept();

  if (client)
  {
    String header = "";
    String cur_line = "";
    unsigned long cur_time = millis();

    while (client.connected() && millis() - cur_time < 500)
    {
      if (client.available())
      {
        char c = client.read();
        header += c;

        if (c == '\n')
        {
          if (cur_line.length() == 0)
          {
            // 1. Маршрут для отримання JSON-даних (викликається фоново з JS)
            if (header.indexOf("GET /data") >= 0)
            {
              float hum = dht.readHumidity();
              float temp = dht.readTemperature();

              client.println("HTTP/1.1 200 OK");
              client.println("Content-type: application/json");
              client.println("Connection: close");
              client.println();

              // Формуємо JSON відповідь
              String json = "{\"temp\": ";
              json += isnan(temp) ? "null" : String(temp, 1);
              json += ", \"hum\": ";
              json += isnan(hum) ? "null" : String(hum, 1);
              json += "}";

              client.print(json);
            }
            // 2. Головна HTML сторінка
            else
            {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type: text/html");
              client.println("Connection: close");
              client.println();

              client.println("<!DOCTYPE html><html>");
              client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
              client.println("<title>ESP8266 Live Weather</title>");
              client.println("<style>");
              client.println("body { font-family: Helvetica, Arial, sans-serif; text-align: center; margin-top: 50px; background-color: #f4f4f9; }");
              client.println(".card { background: white; display: inline-block; padding: 20px 40px; border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }");
              client.println("h1 { color: #333; }");
              client.println(".val { font-size: 2em; color: #007bff; font-weight: bold; }");
              client.println("</style></head>");

              client.println("<body>");
              client.println("<div class=\"card\">");
              client.println("<h1>ESP8266 Weather Station</h1>");
              client.println("<p>Temperature: <span id=\"temp\" class=\"val\">--</span> &deg;C</p>");
              client.println("<p>Humidity: <span id=\"hum\" class=\"val\">--</span> %</p>");
              client.println("</div>");

              // JavaScript для автооновлення даних кожні 2 секунди
              client.println("<script>");
              client.println("function updateData() {");
              client.println("  fetch('/data')");
              client.println("    .then(response => response.json())");
              client.println("    .then(data => {");
              client.println("      if (data.temp !== null) document.getElementById('temp').innerText = data.temp;");
              client.println("      if (data.hum !== null) document.getElementById('hum').innerText = data.hum;");
              client.println("    })");
              client.println("    .catch(err => console.error('Error fetching data:', err));");
              client.println("}");
              client.println("setInterval(updateData, 1000);"); // Оновлення кожні 2000 мс (2 сек)
              client.println("updateData();");                  // Перший виклик одразу при завантаженні
              client.println("</script>");

              client.println("</body></html>");
            }

            break;
          }
          else
          {
            cur_line = "";
          }
        }
        else if (c != '\r')
        {
          cur_line += c;
        }
      }
      yield();
    }

    client.stop();
  }
}
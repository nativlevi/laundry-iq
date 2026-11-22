#include "TelegramNotifier.h"

TelegramNotifier::TelegramNotifier(const char* botToken, const char* chatId)
    : _botToken(botToken)
    , _chatId(chatId)
{
}

void TelegramNotifier::onStateChange(MachineState newState) {
    String message;

    switch (newState) {
        case MachineState::RUNNING:
            message = "🔄 המכונה התחילה לעבוד!";
            break;
        case MachineState::DONE:
            message = "🧺 הכביסה הסתיימה! אפשר לבוא לתלות.";
            break;
        default:
            return;  // IDLE — no notification needed
    }

    Serial.printf("[Telegram] Sending: %s\n", message.c_str());
    bool success = sendMessage(message);

    if (success) {
        Serial.println("[Telegram] Alert sent successfully!");
    } else {
        Serial.println("[Telegram] Failed to send alert.");
    }
}

// ---------------------------------------------------------------
// sendMessage() — HTTP POST with JSON body to Telegram Bot API.
//
// Changed from GET to POST because:
// - GET puts the message in the URL → breaks with Hebrew/emoji
// - POST sends it in the body as JSON with UTF-8 encoding
// ---------------------------------------------------------------
bool TelegramNotifier::sendMessage(const String& message) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[Telegram] WiFi not connected, can't send.");
        return false;
    }

    HTTPClient http;

    String url = "https://api.telegram.org/bot" + _botToken + "/sendMessage";

    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    // Build JSON body
    String jsonBody = "{\"chat_id\":\"" + _chatId + "\",\"text\":\"" + message + "\"}";

    Serial.println("[Telegram] Sending POST...");
    int httpCode = http.POST(jsonBody);

    if (httpCode == 200) {
        Serial.println("[Telegram] HTTP 200 OK");
        http.end();
        return true;
    } else {
        Serial.printf("[Telegram] HTTP error: %d\n", httpCode);
        Serial.println("[Telegram] Response: " + http.getString());
        http.end();
        return false;
    }
}

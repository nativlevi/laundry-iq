#pragma once
#include <HTTPClient.h>
#include "IObserver.h"

/**
 * @brief Concrete Observer — sends Telegram messages on state changes.
 *
 * Implements the IObserver interface.
 * Uses the Telegram Bot API (simple HTTPS GET request).
 *
 * To set up your Telegram bot:
 * 1. Message @BotFather on Telegram → /newbot → get your BOT_TOKEN
 * 2. Message your new bot, then visit:
 *    https://api.telegram.org/bot<YOUR_TOKEN>/getUpdates
 *    → find your chat_id in the response
 *
 * Interview Q: "Why not just put the HTTP code directly in main?"
 * A: "Single Responsibility. TelegramNotifier does ONE thing:
 *     format and send Telegram messages. If the API changes,
 *     or we switch to a different messaging service, we change
 *     one class. The AlertManager and state machine are untouched."
 */
class TelegramNotifier : public IObserver {
public:
    /**
     * @param botToken  Telegram bot token from @BotFather
     * @param chatId    Your Telegram chat ID (numeric string)
     */
    TelegramNotifier(const char* botToken, const char* chatId);

    /**
     * @brief Called by AlertManager when machine state changes.
     *
     * Only sends a message on DONE state — we don't need alerts
     * for IDLE or RUNNING (the user already knows they started it).
     */
    void onStateChange(MachineState newState) override;

private:
    String _botToken;
    String _chatId;

    /**
     * @brief Send a message via Telegram Bot API.
     * @param message Text to send.
     * @return true if HTTP request succeeded.
     */
    bool sendMessage(const String& message);
};

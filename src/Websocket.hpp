/*
 * Websocket.hpp
 *
 *  Created on: Oct 30, 2019
 *      Author: elchaschab
 */

#ifndef SRC_WEBSOCKET_HPP_
#define SRC_WEBSOCKET_HPP_

#include <cstdint>
#include <cstddef>
#include <string>
#include <set>
#include <memory>
#include <iomanip>
#include <mutex>
#include "App.h"
#include "PolySynth.hpp"

std::string escape_json(const std::string &s);

namespace midipatch {

class Websocket {
	us_listen_socket_t* socket_;
	std::set<uWS::WebSocket<false, true>*> clients_;
	std::vector<std::string> buffers_;
	std::mutex mutex_;
	bool audioStreamEnabled_ = false;
	bool restart_ = false;
	std::function<void(string, float)> setControlCallback_;
	std::function<void(size_t, size_t)> noteOnCallback_;
	std::function<void(size_t)> noteOffCallback_;
	std::function<string()> sendControlListCallback_;
	std::function<string()> sendConfigCallback_;

public:
	Websocket(size_t port, const string& logFile, const string& bankFile);
	virtual ~Websocket();
	void setSendControlListCallback(std::function<string()> callback) {
		sendControlListCallback_ = callback;
	}

	void setSetControlCallback(std::function<void(string, float)> callback) {
		setControlCallback_ = callback;
	}

	void setNoteOnCallback(std::function<void(size_t, size_t)> callback) {
		noteOnCallback_ = callback;
	}

	void setNoteOffCallback(std::function<void(size_t)> callback) {
		noteOffCallback_ = callback;
	}

	void setSendConfigCallback(std::function<string()> callback) {
		sendConfigCallback_ = callback;
	}

	void clear();
	void print(const uint8_t& col, const uint8_t& row, const std::string& s);
	void flush();
	void sendAudio(int16_t* audioBuffer, size_t len);
	void updateParameter(const string& name, const float& value);
	bool isAudioStreamEnabled() {
		return audioStreamEnabled_;
	}

	bool isRestartRequested() {
		return restart_;
	}

	void reset() {
		audioStreamEnabled_ = false;
		restart_ = false;
	}
	void sendConfig();
	void sendControlList();
};

} /* namespace midipatch */

#endif /* SRC_WEBSOCKET_HPP_ */

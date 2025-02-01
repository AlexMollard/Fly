#include "MP3Player.h"

MP3Player::MP3Player() : isPlaying(false), volume(80.0f), currentTrackIndex(0), repeat(false), shuffle(false), bass(50.0f), treble(50.0f) {
	music.setVolume(volume);
}

bool MP3Player::loadTrack(const std::string& filename) {
	if (!music.openFromFile(filename)) {
		std::cout << "Error: Could not load " << filename << std::endl;
		return false;
	}
	currentTrack = filename;
	duration = music.getDuration();

	// Add to playlist if not already present
	if (std::find(playlist.begin(), playlist.end(), filename) == playlist.end()) {
		playlist.push_back(filename);
		currentTrackIndex = playlist.size() - 1;
	}
	return true;
}

void MP3Player::playNext() {
	if (playlist.empty()) return;

	if (shuffle) {
		currentTrackIndex = rand() % playlist.size();
	}
	else {
		currentTrackIndex = (currentTrackIndex + 1) % playlist.size();
	}

	loadTrack(playlist[currentTrackIndex]);
	play();
}

void MP3Player::playPrevious() {
	if (playlist.empty()) return;

	if (currentTrackIndex == 0) {
		currentTrackIndex = playlist.size() - 1;
	}
	else {
		currentTrackIndex--;
	}

	loadTrack(playlist[currentTrackIndex]);
	play();
}

void MP3Player::clearPlaylist() {
	stop();
	playlist.clear();
	currentTrack = "";
	currentTrackIndex = 0;
}

void MP3Player::toggleRepeat() { repeat = !repeat; }

void MP3Player::toggleShuffle() { shuffle = !shuffle; }

bool MP3Player::isRepeatEnabled() const { return repeat; }

bool MP3Player::isShuffleEnabled() const { return shuffle; }

void MP3Player::play() {
	if (!currentTrack.empty()) {
		music.play();
		isPlaying = true;
	}
}

void MP3Player::pause() {
	if (isPlaying) {
		music.pause();
		isPlaying = false;
	}
}

void MP3Player::stop() {
	if (!currentTrack.empty()) {
		music.stop();
		isPlaying = false;
	}
}

void MP3Player::setVolume(float newVolume) {
	volume = newVolume;
	music.setVolume(volume);
}

float MP3Player::getVolume() const { return volume; }

bool MP3Player::getIsPlaying() const { return isPlaying; }

std::string MP3Player::getCurrentTrack() const { return currentTrack; }

sf::Time MP3Player::getDuration() const { return duration; }

sf::Time MP3Player::getCurrentTime() const { return music.getPlayingOffset(); }

const std::vector<std::string>& MP3Player::getPlaylist() const { return playlist; }

void MP3Player::setCurrentTime(float percentage) {
	if (!currentTrack.empty()) {
		sf::Time newTime = sf::seconds(duration.asSeconds() * (percentage / 100.0f));
		music.setPlayingOffset(newTime);
	}
}

const char* MP3Player::getFilePath() const { return filepath; }

void MP3Player::setFilePath(const char* path) {
	strncpy_s(filepath, path, sizeof(filepath) - 1);
	filepath[sizeof(filepath) - 1] = '\0';
}

void MP3Player::setBass(float value) {
	bass = value;
}

void MP3Player::setTreble(float value) {
	treble = value;
}

float MP3Player::getBass() const { return bass; }
float MP3Player::getTreble() const { return treble; }
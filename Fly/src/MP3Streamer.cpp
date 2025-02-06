#include "pch.h"

#include "MP3Streamer.h"

#include <stdexcept>

MP3Streamer::MP3Streamer()
{
	// Initialize sample buffer
	m_sampleBuffer.resize(BUFFER_SIZE);
}

MP3Streamer::~MP3Streamer()
{
	cleanup();
}

MP3Streamer::MP3Streamer(MP3Streamer&& other) noexcept
      : AudioStreamer(std::move(other)), m_file(std::exchange(other.m_file, nullptr)), m_fileInfo(other.m_fileInfo), m_sampleBuffer(std::move(other.m_sampleBuffer))
{
	// Clear other's file info
	std::memset(&other.m_fileInfo, 0, sizeof(SF_INFO));
}

MP3Streamer& MP3Streamer::operator=(MP3Streamer&& other) noexcept
{
	if (this != &other)
	{
		AudioStreamer::operator=(std::move(other));
		cleanup();

		m_file = std::exchange(other.m_file, nullptr);
		m_fileInfo = other.m_fileInfo;
		m_sampleBuffer = std::move(other.m_sampleBuffer);

		// Clear other's file info
		std::memset(&other.m_fileInfo, 0, sizeof(SF_INFO));
	}
	return *this;
}

bool MP3Streamer::openFromFile(const std::string& filename)
{
	// Cleanup any existing file
	cleanup();

	// Initialize file info
	std::memset(&m_fileInfo, 0, sizeof(SF_INFO));

	// Open the audio file
	m_file = sf_open(filename.c_str(), SFM_READ, &m_fileInfo);
	if (!m_file)
	{
		throw std::runtime_error("Failed to open audio file: " + std::string(sf_strerror(nullptr)));
	}

	// Configure audio streamer
	StreamingConfig config;
	config.channelCount = static_cast<unsigned int>(m_fileInfo.channels);
	config.sampleRate = static_cast<unsigned int>(m_fileInfo.samplerate);
	config.bufferSize = BUFFER_SIZE;
	initialize(config);

	// Setup the trackInfo with null checks
	const char* title = sf_get_string(m_file, SF_STR_TITLE);
	m_trackInfo.title = title ? title : filename.substr(filename.find_last_of("/\\") + 1); // If there is no title, use the trimmed filename

	m_trackInfo.artist = sf_get_string(m_file, SF_STR_ARTIST) ? sf_get_string(m_file, SF_STR_ARTIST) : "";
	m_trackInfo.album = sf_get_string(m_file, SF_STR_ALBUM) ? sf_get_string(m_file, SF_STR_ALBUM) : "";
	m_trackInfo.genre = sf_get_string(m_file, SF_STR_GENRE) ? sf_get_string(m_file, SF_STR_GENRE) : "";
	m_trackInfo.year = sf_get_string(m_file, SF_STR_DATE) ? sf_get_string(m_file, SF_STR_DATE) : "";

	// How do I get the duration?
	m_trackInfo.duration = static_cast<float>(m_fileInfo.frames) / m_fileInfo.samplerate;

	return true;
}

void MP3Streamer::cleanup()
{
	if (m_file)
	{
		sf_close(m_file);
		m_file = nullptr;
	}
	std::memset(&m_fileInfo, 0, sizeof(SF_INFO));
}

void MP3Streamer::close()
{
	stop();
	cleanup();
}

const AudioStreamer::TrackInfo& MP3Streamer::GetTrackInfo()
{
	return m_trackInfo;
}

bool MP3Streamer::onGetData(AudioChunk& chunk)
{
	if (!m_file)
		return false;

	// Read audio data
	static sf_count_t framesToRead = BUFFER_SIZE / m_fileInfo.channels;
	sf_count_t framesRead = sf_readf_float(m_file, m_sampleBuffer.data(), framesToRead);

	if (framesRead > 0)
	{
		chunk.samples = m_sampleBuffer.data();
		chunk.sampleCount = static_cast<std::size_t>(framesRead * m_fileInfo.channels);
		return true;
	}

	// End of file reached
	return false;
}

void MP3Streamer::onSeek(double timeOffset)
{
	if (!m_file)
		return;

	sf_count_t frame = static_cast<sf_count_t>(timeOffset * m_fileInfo.samplerate);

	// Clamp the frame position
	frame = std::clamp(frame, static_cast<sf_count_t>(0), m_fileInfo.frames);

	// Seek to the frame
	sf_seek(m_file, frame, SEEK_SET);
}

float MP3Streamer::onGetDuration() const
{
	if (!m_file)
		return 0;

	return m_trackInfo.duration;
}

std::optional<std::size_t> MP3Streamer::onLoop()
{
	if (!m_file)
		return std::nullopt;

	// Seek back to start
	sf_seek(m_file, 0, SEEK_SET);

	// Return total number of samples processed so far
	return 0;
}

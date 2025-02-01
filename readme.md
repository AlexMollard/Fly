# ImGui MP3 Player

A modern, minimal MP3 player built with Dear ImGui and OpenAL. Features a clean, Gruvbox-themed interface with basic audio controls and filtering capabilities.

![alt text](res/screenshot.png)

## Features

- Clean, modern Gruvbox-themed interface
- Basic playback controls (play, pause, stop, next, previous)
- Volume control with visual feedback
- Bass and treble adjustment
- Playlist management
- File browser for easy track selection
- Progress bar with time display
- Shuffle and repeat modes

## Dependencies

- Dear ImGui
- OpenAL Soft
- hello_imgui
- Lucide Icons

### Plan:
1. Remove unnecessary filepath comment
2. Fix markdown formatting for numbered list
3. Improve section heading structure
4. Clean up code block formatting
5. Add integration instructions for vcpkg

## Building

### 1. Clone the repository
```bash
git clone https://github.com/yourusername/imgui-mp3-player.git
cd imgui-mp3-player
```

### 2. Install dependencies
Install dependencies using vcpkg package manager:
```bash
vcpkg install openal-soft
vcpkg install "hello-imgui[opengl3-binding,glfw-binding]"
```

### 3. Build the project
1. Open the solution file in Visual Studio:
   ```bash
   start imgui-mp3-player.sln
   ```
2. Select your preferred configuration (Debug/Release)
3. Build the project (F7 or Build > Build Solution)
4. Ensure the working directory is set to the project directory

Note: Make sure vcpkg is integrated with Visual Studio. Run:
```bash
vcpkg integrate install
```

## Usage

1. Launch the application
2. Click "Open File" to add music to your playlist
3. Use the playback controls to control your music
4. Adjust volume, bass, and treble using the sliders
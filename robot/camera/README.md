# Tutorial
Dedicated **only** for Linux 🐧

## Installation [Debian]
1. Update packages
    ```bash
    sudo apt-get update
    sudo apt-get upgrade
    ```
2. Install prerequisites
    ```bash
    sudo apt-get install build-essential cmake libspdlog-dev libasio-dev libgstrtspserver-1.0-dev libgstreamer1.0-dev libyaml-cpp-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio
    ```
3. Inside Visual Studio Code
    - Click on `File > Open Workspace from File...`
    - Open `camera.code-workspace`
4. You've installed the project 🎉

## Compile & run
1. Run `build.sh` script
2. Inside the `build` directory there would be `Camera` executable

## Testing
If you **only** want to test the current software, you could use the `Dockerfile` image.

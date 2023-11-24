#include "VideoStream.hpp"
#include <stdexcept>
#include <chrono>

bool VideoStream::IsArgumentsCountValid(const std::vector<std::string> &arguments, int expected)
{
    if (arguments.size() != expected)
    {
        std::string argumentList = "";
        for (auto arg : arguments)
        {
            argumentList += arg + "; ";
        }

        log.Error("Expected " + std::to_string(expected) + " argument(s), got " + 
            std::to_string(arguments.size()) + ": " + argumentList);
        return false;
    }

    return true;
}

void VideoStream::HandleCommand(const std::string &command)
{
    auto commandText = std::istringstream(command);
    auto tokens = std::vector<std::string>
    {
        std::istream_iterator<std::string>
        {
            commandText
        },
        std::istream_iterator<std::string>{}
    };

    auto searchedCommand = this->command.find(tokens[0]);
    if (searchedCommand != this->command.end())
    {
        auto args = std::vector<std::string>(tokens.begin() + 1, tokens.end());
        searchedCommand->second(args);
    }
    else
    {
        log.Warning("Received unknown command: " + command + ". Ignoring.");
    }
}

void VideoStream::HandleRequest(std::shared_ptr<asio::ip::tcp::socket> socket)
{
    auto buffer = std::make_shared<asio::streambuf>();
    asio::async_read_until(*socket, *buffer, "\n", 
        [this, buffer, socket](const asio::error_code& error, size_t)
        {
            if (error)
            {
                log.Error("Error while retreiving client message: " + error.message());
                socket->close();

                return;
            }

            std::istream input(buffer.get());
            std::string line;
            std::getline(input, line);

            this->HandleCommand(line);
            
            socket->close();
        }
    );
}

void VideoStream::ListenForRequests()
{
    acceptor->async_accept(
        [this](const asio::error_code& error, asio::ip::tcp::socket socket) 
        {
            if (error)
            {
                log.Error("Error while setting up endpoint: " + error.message());
                return;
            }

            // Get the remote endpoint's IP address and port
            std::string remoteIP = socket.remote_endpoint().address().to_string();
            unsigned short remotePort = socket.remote_endpoint().port();
            log.Debug("Received request from: " + remoteIP + ":" + std::to_string(remotePort));
            
            this->HandleRequest(
                std::make_shared<asio::ip::tcp::socket>(std::move(socket))
            );
        }
    );
}

VideoStream::VideoStream()
{
    this->command["START"] = [this](const std::vector<std::string>&)
    {
        if (this->ipAddress.empty())
        {
            log.Error("Cannot start streaming as ADDRESS wasn't specified!");
            return;
        }

        if (this->port.empty())
        {
            log.Error("Cannot start streaming as PORT wasn't specified!");
            return;
        }

        if (!gstreamer.IsStreaming())
        {
            gstreamer.BuildPipeline(this->ipAddress, this->port);
            gstreamer.Start();
        }

        log.Info("Streaming started!");
    };

    this->command["STOP"] = [this](const std::vector<std::string>&)
    {
        if (gstreamer.IsStreaming())
        {
            gstreamer.Stop();
        }

        log.Info("Streaming stopped!");
    };

    this->command["EXIT"] = [this](const std::vector<std::string>&)
    {
        log.Info("Quitting...");

        this->listenToClient.store(false);
    };

    this->command["SET"] = [this](const std::vector<std::string>& args)
    {
        if (!this->IsArgumentsCountValid(args, 2))
        {
            return;
        }

        if (args[0] == "ADDRESS")
        {
            this->ipAddress = args[1];
            log.Info("New IP address " + args[1] + " set!");
        }
        else if (args[0] == "PORT")
        {
            this->port = args[1];
            log.Info("New port " + args[1] + " set!");
        }
        else
        {
            log.Warning("Unknown parameter: " + args[0] + ". Aborting!");
        }
    };

    this->command["USE"] = [this](const std::vector<std::string>& args)
    {
        if (args.empty())
        {
            this->log.Error("Expected pipeline argument!");
            return;
        }

        std::string pipeline = "";
        for (auto arg : args)
        {
            pipeline += arg + " ";
        }

        if (gstreamer.IsStreaming())
        {
            gstreamer.Stop();
            gstreamer.SetPipeline(pipeline);
            gstreamer.Start();
        }
        else
        {
            gstreamer.SetPipeline(pipeline);
        }
    };
}

void VideoStream::ListenOn(const std::string &serverIp, unsigned short port)
{
    log.Info("Binding to " + serverIp + ":" + std::to_string(port) + "...");

    acceptor = std::make_unique<asio::ip::tcp::acceptor>(
        this->clientContext, asio::ip::tcp::endpoint(asio::ip::make_address(serverIp), port)
    );

    this->listenToClient.store(true);
    this->listenerThread = std::thread([this]() 
    {
        while (this->listenToClient.load())
        {
            this->ListenForRequests();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            this->clientContext.run();
            this->clientContext.reset();
        }
    });
}

bool VideoStream::IsListening()
{
    return this->listenToClient.load();
}

VideoStream::~VideoStream()
{
    listenToClient.store(false);

    clientContext.stop();

    if (listenerThread.joinable())
    {
        listenerThread.join();
    }

    log.Info("Stream closed!");
}

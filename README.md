# ShadowStrike
[![Screenshot-from-2024-08-07-19-12-14.png](https://i.postimg.cc/d33ShbcG/Screenshot-from-2024-08-07-19-12-14.png)](https://postimg.cc/m1xSq6Dr)


## Overview
The Facebook Account Reporter is a command-line tool designed to report Facebook accounts for abuse. It supports multiple features such as multi-account reporting, configurable retries and delays, proxy support, enhanced error handling, and interactive mode with colorful output. This tool utilizes the Facebook Graph API for reporting.

### Features
- Configurable Retries and Delays: Customize the number of retries and the delay between retries.
- Interactive Mode: User-friendly interactive mode for inputting data with colorful prompts.
- Multi-Account Reporting: Report multiple accounts by loading account IDs from a configuration file.
- Additional Reporting Parameters: Includes fields for reason and category for reporting.
- Enhanced Error Handling: Improved error messages and logging.
- Command Line Arguments: Supports command-line options for configuration file input and interactive mode.
- Configurable File Input: Load and save configuration to a file.
- Input Encryption: Hides sensitive input like access tokens during entry.
- Unlimited & Continuous Reporting: Continuous reporting on a single account with configurable retries and delays.

### Prerequisites
- GCC compiler
- libcurl development files (`libcurl-dev` package)
- pthread library (`libpthread` package)

### Installation
1.Clone the repository:
```
https://github.com/NumbleFox/ShadowStrike.git
cd ShadowStrike
```
2.Compile the program:
```
gcc -o facebook_reporter facebook_reporter.c -lcurl -lpthread
```

### Usage
#### Command Line Arguments
- `-c <config_file>`: Load configuration from a file.
- `-i`: Run in interactive mode.

### Running the Program
#### Using a configuration file:
```
./facebook_reporter -c config.txt
```

#### Interactive mode:
```
./facebook_reporter -i
```

#### Configuration File Format
The configuration file should contain the following parameters in the specified order:
```
<account_id> <access_token> <reason> <category> <proxy> <retries> <delay>
```

Example:
```
1234567890 youraccesstoken inappropriate_content fake_account http://proxyserver:port 5 2
```

### Example Usage
##### Interactive Mode:
```
./facebook_reporter -i
```
Follow the prompts to input the necessary information. The access token input will be hidden for security purposes.

#### Configuration File Mode:

1.Create a configuration file (`config.txt`) with the necessary parameters:
```
1234567890 youraccesstoken inappropriate_content fake_account http://proxyserver:port 5 2
```

2.Run the program:
```
./facebook_reporter -c config.txt
```


### Logs
The program logs all actions to a file named `facebook_report.log` in the current directory. Each log entry includes a timestamp and a message describing the action taken.

### Contributing
- Fork the repository.
- Create your feature branch (git checkout -b feature/AmazingFeature).
- Commit your changes (git commit -m 'Add some AmazingFeature').
- Push to the branch (git push origin feature/AmazingFeature).
- Open a Pull Request.


### License
Distributed under the MIT License. See `LICENSE` for more information.
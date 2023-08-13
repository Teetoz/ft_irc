#include "Parser.hpp"

Parser::Parser() {
	func["NICK"] = &Parser::NICK;
	func["CAP"] = &Parser::CAP;
	func["JOIN"] = &Parser::JOIN;
	func["USER"] = &Parser::USER;
	func["MODE"] = &Parser::MODE;
	func["PING"] = &Parser::PING;
	// func["PART"] = &Parser::PART;
	func["PRIVMSG"] = &Parser::PRIVMSG;
	func["PASS"] = &Parser::PASS;
	func["MOTD"] = &Parser::MOTD;
	func["QUIT"] = &Parser::QUIT;
	func["NOTICE"] = &Parser::NOTICE;
	func["WHOIS"] = &Parser::WHOIS;
	func["OPER"] = &Parser::OPER;
	func["KILL"] = &Parser::KILL;
}

Parser::~Parser() {}

void Parser::takeInput( std::string Input, int fd, Client &client ) {
	this->input = Input;
	this->fd = fd;
	this->findCmdArgs();
	current_client = &client;
	try {
		this->excecuteCommand();
		nicks[current_client->getNick()] = *current_client;
	} catch (std::exception &e) {
		std::cout << "Exception caught: " << e.what();
	}
}

void Parser::findPass( void ) {
	std::string::size_type pos = input.find_first_of("\r\n");
	if (pos != input.npos) {
		args.push_back(input.substr(0, pos));
		input.erase(0, pos + 2);
		multi_cmd.push_back(args);
		args.clear();
	}
}

void Parser::findCmdArgs( void ) {
	std::string::size_type pos;

	args.clear();
	multi_cmd.clear();
	while (!input.empty()) {
		if (input[0] == ':' && (pos = input.find_first_of("\r\n")) != input.npos) {
			args.push_back(input.substr(1, pos - 1));
			input.erase(0, pos + 1);
		} else if ((pos = input.find_first_of(" \r\n")) != input.npos) {
			args.push_back(input.substr(0, pos));
			input.erase(0, pos + 1);
		} else {
			args.push_back(input);
			input.clear();
		}
		if (makeUpper(args.front()) == "PASS")
			this->findPass();
		else if (args.back().empty() || input.empty()) {
			if (args.back().empty())
				args.pop_back();
			multi_cmd.push_back(args);
			args.clear();
		}
	}
	for (size_t i = 0; i < multi_cmd.size(); i++) {
		for (size_t j = 0; j < multi_cmd[i].size(); j++) {
			std::cout << "|" << multi_cmd[i][j] << "|" << std::endl;
		}
		std::cout << std::endl;
	}
}

std::string Parser::makeUpper( std::string str) {
	std::string dest = str;
	std::transform(str.begin(), str.end(), dest.begin(), toupper);
	return dest;
}

void Parser::excecuteCommand( void ) {
	std::map<std::string, Funcs>::iterator command;

	while (!multi_cmd.empty() && !multi_cmd.front().empty()) {
		cmd = makeUpper(multi_cmd.front().front());
		multi_cmd.front().pop_front();
		args = multi_cmd.front();
		multi_cmd.pop_front();
		command = func.find(cmd);
		if (command != func.end()) {
			if (checkRegistration())
				(this->*command->second)();
			else
				ServerMessage(ERR_NOTREGISTERED, " :need to register first\n", *current_client);
		}
		else
			ServerMessage(ERR_UNKNOWNCOMMAND, " :command not found\n", *current_client);
	}
}

bool Parser::checkRegistration( void ) {
	if (cmd != "PASS" && cmd != "CAP" && cmd !="USER" && cmd != "NICK") {
		if (current_client->isPassGood() && current_client->isRegistered())
			return true;
		return false;
	}
	return true;
}

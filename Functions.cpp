#include "Functions.hpp"
#include "Commands.hpp"


Functions::Functions( ) : operPass("DamnSon"){

}

Functions::~Functions( ) {

}

void Functions::setPass( std::string pass ) {
	this->pass = pass;
}

std::string Functions::getPass( void ) const {
	return this->pass;
}

void Functions::addNick( std::string nick ) {
	client_it it;

	it = nicks.find(nick);
	if (nick.find_first_of("#&\x03") != nick.npos)
		ServerMessage(ERR_ERRONEUSNICKNAME, ":" + nick + " erroneus nick name\n", *current_client);
	else if (it != nicks.end()) {
		if (current_client->isRegistered())
			ServerMessage(ERR_NICKNAMEINUSE, nick + " :Nickname is already in use\n", *current_client);
		else {
			ServerMessage("ERROR ", "13 your nick is unavailable, get a better name and try connecting again\n", *current_client);
			multi_cmd.clear();
			close(fd);
			throw IrcErrorException("user tried to register with nick already in use\n");
		}
	} else {
		if (nick.length() > 16)
			nick = nick.substr(0, 16);
		updateChannel(*current_client, current_client->getNick(), nick);
		UserMessage(cmd, " " + nick + " :" + nick + "\n", *current_client);
		nicks.erase(current_client->getNick());
		current_client->setNick(nick);
	}
}

void Functions::updateChannel( Client &client, std::string old_nick, std::string new_nick ) {
	for(chan_it it = channels.begin(); it != channels.end(); it++) {
		if (it->second.isInChan(old_nick)) {
			it->second.updateMemberNick(client, old_nick, new_nick);
			it->second.echoToAll(*current_client, cmd, ":" + new_nick, false, sent);
		}
	}
	sent.clear();
}

void Functions::RegisterUser( void ) {
	try {
		if (current_client->getUserName().empty())
			current_client->setUserName("~" + args.at(0));
		if (current_client->getHostName().empty())
			current_client->setHostName(args.at(1));
		current_client->setServerName(args.at(2));
		current_client->setRealName(args.at(3));
		if (current_client->getNick().empty())
			ServerMessage(ERR_NONICKNAMEGIVEN, ":no nick name given\n", *current_client);
		else {
			current_client->registration();
			ConnectionMessage(*current_client);
			// this->MOTD();
		}
	} catch (std::exception &e) {
		ServerMessage(ERR_NEEDMOREPARAMS, ":need more params\n", *current_client);
		nicks.erase(current_client->getNick());
		close(fd);
		throw IrcErrorException("user not registered\n");
	}
}

void Functions::killMsg(Client source, Client dest) {
	std::string message = ":" + USER_FN(source.getNick(), source.getUserName(), source.getHostName());
	message += " " + cmd + " " + dest.getNick() + " " + args[1] + "\n";
	std::cout << message << std::endl;
	send(dest.getFD(), message.data(), message.length(), 0);
}

void Functions::quitMsg(Client source, std::string msg)
{
	std::string nick = source.getNick();
	std::string user_info = USER_FN(source.getNick(), source.getUserName(), source.getHostName());
	std::string mes = ":" + user_info + " QUIT :Quit: " + msg;
	send(source.getFD(), mes.data(), mes.length(), 0);
	for (chan_it it = channels.begin(); it != channels.end(); it++) {
		if (it->second.isInChan(nick)) {
			it->second.echoToAll(source, "", mes, false, sent);
			it->second.removeMember(source);
		}
	}
	sent.clear();
}

void Functions::errMsg(client_it dest, std::string msg)
{
	std::string mes = ":" + dest->second.getServerName() + "Error: " + msg + "\n";
	send(dest->second.getFD(), mes.data(), mes.length(), 0);
	close(dest->second.getFD());
	nicks.erase(dest);
}

bool Functions::isEnoughParams(unsigned int paramNum)
{
	if (args.size() < paramNum)
	{
		ServerMessage(ERR_NEEDMOREPARAMS, " :need more params\n", *current_client);
		return false;
	}
	return true;
}

bool Functions::channelExist(std::string chanName, chan_it channel)
{
	if (channel == channels.end())
	{
		ServerMessage(ERR_NOSUCHCHANNEL, chanName + " :No such channel\n", *current_client);
		return false;
	}
	return true;
}

bool Functions::userInChan(std::string chanName, chan_it channel)
{
	if (!channel->second.isInChan(current_client->getNick()))
	{
		ServerMessage(ERR_NOTONCHANNEL, chanName + " :You're not on that channel\n", *current_client);
		return false;
	}
	return true;
}

bool Functions::isUserOp(std::string chanName)
{
	if (!current_client->isOperator())
	{
		ServerMessage(ERR_CHANOPRIVSNEEDED, chanName + " :You're not a channel operator\n", *current_client);
		return false;
	}
	return true;
}
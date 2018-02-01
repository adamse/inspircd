#include "inspircd.h"
#include "cbor11.h"
#include "cbor11.cpp"

class MyUser : public User {
public:

  MyUser(std::string withNick);

  void SendText(const std::string& line);
  void Write(const std::string& text);

  void Say(Channel* ch, const std::string& text);
};

MyUser::MyUser(std::string name)
  : User(ServerInstance->GetUID(), ServerInstance->Config->ServerName, 4) {

  this->ChangeDisplayedHost("telegram");
  this->ChangeIdent("ident");
  this->ChangeName("ident");
  this->ChangeNick(name);
  this->registered = REG_ALL;
}

void MyUser::Write(const std::string& text) {
  ServerInstance->Logs->Log("m_adam", DEFAULT, "MyUser %s MSG %s", nick.c_str(), text.c_str());
}

void MyUser::SendText(const std::string& line) {
  Write(line);
}

void MyUser::Say(Channel* ch, const std::string& text) {
  ch->WriteChannel(this, "PRIVMSG %s :%s", ch->name.c_str(), text.c_str());
}

typedef std::set<MyUser*> Users;
typedef std::set<MyUser*>::iterator UsersIt;

typedef std::set<Channel*> Channels;
typedef std::set<Channel*>::iterator ChannelsIt;

enum TGMessageType {
  TG_JOIN = 0, // Join channel
  TG_ADDUSER, // Add user
  TG_IRCMESSAGE, // Message from IRC channel
  TG_MESSAGE, // Telegram message
};


class ModuleAdam : public Module {
private:
public:

  Users users;
  Channels channels;


  void init() {

    ServerInstance->Modules->Attach(I_OnUserMessage, this);
    ServerInstance->Modules->Attach(I_OnUnloadModule, this);
    ServerInstance->Modules->Attach(I_OnAcceptConnection, this);

    addChannel("#test");
    addUser("user1");
    addUser("user2");

    cbor item = cbor::tagged (TG_JOIN, cbor::array {12, 1.2});


    // Convert to diagnostic notation for easy debugging
    // std::cout << cbor::debug (item) << std::endl;

    // Encode
    cbor::binary data = cbor::encode (item);

    // Decode (if invalid data is given cbor::undefined is returned)
    item = cbor::decode (data);

    // Read from any instance of std::istream
    // item.read (std::cin);

    // // Write to any instance of std::ostream
    // item.write (std::cout);

    return;
  }

  void OnUnloadModule(Module* mod) {
    if (mod != this)
      return;

    for (UsersIt i = users.begin(); i != users.end(); ++i) {
      ServerInstance->Users->QuitUser(*i, "Unloading m_telegram.so");
    }
  }

  virtual Version GetVersion() { return Version ("Test"); }

  ModResult OnAcceptConnection(int nfd, ListenSocket* from, irc::sockets::sockaddrs* client, irc::sockets::sockaddrs* server) {
    if (from->bind_tag->getString("type") != "telegram")
      return MOD_RES_PASSTHRU;

    return MOD_RES_ALLOW;
  }

  void OnUserMessage(User* user, void* dest, int target_type, const std::string& text, char status, const CUList& exempt_list) {

    if (target_type != TYPE_CHANNEL)
      return;

    Channel* ch = (Channel*) dest;

    if (channels.count(ch) > 0) {
      ServerInstance->Logs->Log("m_adam", DEFAULT, "Meddelande! %s", text.c_str());
    }

  }

  bool addChannel(const char* name) {

    Channel* ch = ServerInstance->FindChan(name);

    if (!ch)
      return false;

    channels.insert(ch);

    return true;

  }

  void addUser(const char* name) {
    MyUser* user = new MyUser(name);

    for (ChannelsIt i = channels.begin(); i != channels.end(); ++i) {
      Channel::JoinUser(user, (*i)->name.c_str(), false, "", false);
      user->Say(*i, "hejsan!");
    }

    users.insert(user);
  }


};

MODULE_INIT(ModuleAdam)

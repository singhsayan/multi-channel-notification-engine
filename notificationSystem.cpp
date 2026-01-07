#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <memory>

using namespace std;

class INotification {
public:
    virtual string getContent() const = 0;
    virtual ~INotification() = default;
};

class SimpleNotification : public INotification {
private:
    string text;
public:
    SimpleNotification(const string& msg) : text(msg) {}
    string getContent() const override {
        return text;
    }
};

class INotificationDecorator : public INotification {
protected:
    unique_ptr<INotification> notification;
public:
    INotificationDecorator(unique_ptr<INotification> n)
        : notification(std::move(n)) {}
};

class TimestampDecorator : public INotificationDecorator {
public:
    TimestampDecorator(unique_ptr<INotification> n)
        : INotificationDecorator(std::move(n)) {}

    string getContent() const override {
        return "[2025-10-26 10:45:00] " + notification->getContent();
    }
};

class SignatureDecorator : public INotificationDecorator {
private:
    string signature;
public:
    SignatureDecorator(unique_ptr<INotification> n, string sig)
        : INotificationDecorator(std::move(n)), signature(std::move(sig)) {}

    string getContent() const override {
        return notification->getContent() + "\n-- " + signature + "\n\n";
    }
};

// Observer Interfaces
class IObserver {
public:
    virtual void update() = 0;
    virtual ~IObserver() = default;
};

class IObservable {
public:
    virtual void addObserver(shared_ptr<IObserver> observer) = 0;
    virtual void removeObserver(shared_ptr<IObserver> observer) = 0;
    virtual void notifyObservers() = 0;
    virtual ~IObservable() = default;
};

// Observable
class NotificationObservable : public IObservable {
private:
    vector<weak_ptr<IObserver>> observers;
    shared_ptr<INotification> currentNotification;

public:
    void addObserver(shared_ptr<IObserver> obs) override {
        observers.push_back(obs);
    }

    void removeObserver(shared_ptr<IObserver> obs) override {
        observers.erase(
            remove_if(
                observers.begin(),
                observers.end(),
                [&](weak_ptr<IObserver>& w) {
                    auto sp = w.lock();
                    return !sp || sp == obs;
                }),
            observers.end()
        );
    }

    void notifyObservers() override {
        for (auto &w : observers) {
            if (auto obs = w.lock()) {
                obs->update();
            }
        }
    }

    void setNotification(shared_ptr<INotification> notification) {
        currentNotification = std::move(notification);
        notifyObservers();
    }

    shared_ptr<INotification> getNotification() {
        return currentNotification;
    }

    string getNotificationContent() {
        return currentNotification->getContent();
    }
};

// Singleton NotificationService
class NotificationService {
private:
    NotificationObservable observable;
    vector<shared_ptr<INotification>> notifications;

    NotificationService() = default;

public:
    static NotificationService& getInstance() {
        static NotificationService instance;
        return instance;
    }

    NotificationObservable* getObservable() {
        return &observable;
    }

    void sendNotification(shared_ptr<INotification> notification) {
        notifications.push_back(notification);
        observable.setNotification(notification);
    }
};

// Logger
class Logger : public IObserver, public enable_shared_from_this<Logger> {
private:
    NotificationObservable* observable;

public:
    Logger() {
        observable = &NotificationService::getInstance().getObservable()[0];
    }

    void subscribe() {
        observable->addObserver(shared_from_this());
    }

    void update() override {
        cout << "\n[Logger] New Notification Logged:\n"
             << observable->getNotificationContent();
    }
};

// Strategy Interface
class INotificationStrategy {
public:
    virtual void sendNotification(const string& content) = 0;
    virtual ~INotificationStrategy() = default;
};

class EmailStrategy : public INotificationStrategy {
private:
    string emailId;
public:
    EmailStrategy(string emailId) : emailId(std::move(emailId)) {}

    void sendNotification(const string& content) override {
        cout << "\n[Email] Sent to " << emailId << ":\n" << content;
    }
};

class SMSStrategy : public INotificationStrategy {
private:
    string mobileNumber;
public:
    SMSStrategy(string mobileNumber) : mobileNumber(std::move(mobileNumber)) {}

    void sendNotification(const string& content) override {
        cout << "\n[SMS] Sent to " << mobileNumber << ":\n" << content;
    }
};

class PopUpStrategy : public INotificationStrategy {
public:
    void sendNotification(const string& content) override {
        cout << "\n[Popup] Notification displayed:\n" << content;
    }
};

// Engine
class NotificationEngine : public IObserver, public enable_shared_from_this<NotificationEngine> {
private:
    NotificationObservable* observable;
    vector<unique_ptr<INotificationStrategy>> strategies;

public:
    NotificationEngine() {
        observable = NotificationService::getInstance().getObservable();
    }

    void subscribe() {
        observable->addObserver(shared_from_this());
    }

    void addNotificationStrategy(unique_ptr<INotificationStrategy> ns) {
        strategies.push_back(std::move(ns));
    }

    void update() override {
        string content = observable->getNotificationContent();
        for (auto &s : strategies) s->sendNotification(content);
    }
};

int main() {
    auto& notificationService = NotificationService::getInstance();

    auto logger = make_shared<Logger>();
    logger->subscribe();

    auto engine = make_shared<NotificationEngine>();
    engine->subscribe();

    engine->addNotificationStrategy(make_unique<EmailStrategy>("abc@outlook.com"));
    engine->addNotificationStrategy(make_unique<SMSStrategy>("+353 8743210"));
    engine->addNotificationStrategy(make_unique<PopUpStrategy>());

    shared_ptr<INotification> notification =
        make_shared<SignatureDecorator>(
            make_unique<TimestampDecorator>(
                make_unique<SimpleNotification>("Your internship confirmation has been approved!")
            ),
            "Microsoft Dublin HR Team"
        );

    notificationService.sendNotification(notification);

    return 0;
}

#ifndef _DDS_SIMPLE_PSM_H
#define _DDS_SIMPLE_PSM_H

#include "dds_manager.h"
#include "std_qosC.h"
#include <typeinfo>
#include <map>
#include <mutex>
#include <unordered_set>
#include <future>
#include <shared_mutex>

class DDSSimpleManager : public DDSManager
{
public:


    /**
    * @brief Constructor for the DDS C++ PSM manager class that sets a class-wide eventID.
    */
    DDSSimpleManager(int eventID = 0, std::function<void(LogMessageType mt, const std::string& message)> messageHandler = nullptr,
        int threadPoolSize = DDSManager::DefaultThreadPoolSize) : DDSManager(messageHandler, threadPoolSize)
    {
        m_eventID = eventID;
    }

    /**
    * @brief Destructor for the DDS C++ PSM manager class.
    */
    ~DDSSimpleManager()
    {
        std::stringstream sstr;
        sstr << "DDSSimpleManager destructor domain:" << GetDomainID() << " event:" << m_eventID << ".";
        m_messageHandler(LogMessageType::DDS_INFO, sstr.str());
    }

    /**
     * @brief Join the DDS domain.
     *
     * @details An INI file is used to configure the DDS library. See chapter 7
     *          of the OpenDDS developer guide for available options. The
     *          DDS_CONFIG environment variable can be used to specify the path
     *          to this file. If DDS_CONFIG is not set, the opendds.ini file
     *          from the working directory will be used.  Using this version of
     *          the method as opposed to calling the base class version causes
     *          participants to be logged in the default manner.
     *
     * @param[in] domainID Join the DDS domain with this ID.
     * @param[in] config Optionally specify the config section from the INI
     *            file for this domain. The config section format is
     *            [config/NAME] where NAME is the input parameter for this
     *            method.
     * @return True if the operation was successful; false otherwise.
     */
    bool joinDomain(const int& domainID, const std::string& config = "") {
        //Define functions for logging participant information to satisfy STIG requirement
        auto joinDomainFn = [this, domain = domainID](const ParticipantInfo& info) {
            std::stringstream sstr;
            sstr << "New participant joined domain " << domain << ".  IP Address:" << info.location;
            sstr << " guid:" << info.guid << " at time:" << info.discovered_timestamp << ".";
            m_messageHandler(LogMessageType::DDS_INFO, sstr.str());
        };

        auto leaveDomainFn = [this, domain = domainID](const ParticipantInfo& info) {
            std::stringstream sstr;
            sstr << "Participant left domain " << domain << ".  IP Address:" << info.location;
            sstr << " guid:" << info.guid << " at time:" << info.discovered_timestamp << ".";
            m_messageHandler(LogMessageType::DDS_INFO, sstr.str());
        };

        return DDSManager::joinDomain(domainID, config, joinDomainFn, leaveDomainFn);
    }

    /**
     * With this version of the function, the on add and on remove functions are supplied.
     * @param[in] onAdd Optional function called when a new participant is added to the domain.
     *            The function argument is a structure with information about the new
     *            participant.  This funciton can be set to null if no function should be called.
     * @param[in] onRemove Optional function called when a participant leaves the domain.
     *            The function argument is a structure with information about the
     *            participant which left.  This funciton can be set to null if no function
     *            should be called.
    */
    bool joinDomain(const int& domainID, const std::string& config, std::function<void(const ParticipantInfo&)> onAdd, std::function<void(const ParticipantInfo&)> onRemove) {
        return DDSManager::joinDomain(domainID, config, onAdd, onRemove);
    }


    ///Example usage: ddsPublisher<STATE::StateStatus>(STATE::STATE_STATUS_TOPIC_NAME);
    template <class T>
    bool Publisher(std::string topicName, const STD_QOS::QosType qos)
    {

        //ProcessLockGuard lock(m_initMutex);

        if (false == registerTopic<T>(topicName, qos))
        {
            //If a topic has already been registered, this will fail but it should not stop us from continuing
        }
        if (false == createPublisher(topicName))
        {
            return false;
        }

        decltype(m_uniqueLock) lck(mutex_shr);
        //Let's try something sneaky, so we don't have to pass topicName when we write DDS messages
        m_pubMap[typeid(T).name()] = topicName;
        return true;
    }

    ///Example usage: ddsSubcriber<STATE::StateStatus>(STATE::STATE_STATUS_TOPIC_NAME, "(eventID = 1)");
    template <class T>
    bool Subscriber(std::string topicName, const STD_QOS::QosType qos,
        std::string filter = "", std::string readerName = "")
    {
        std::string rName = GenerateReaderName(topicName, readerName);

        if (false == registerTopic<T>(topicName, qos))
        {
            //If a topic has already been registered, this will fail but it should not stop us from continuing
        }
        bool temp = createSubscriber(topicName, rName, filter);

        m_subMap[typeid(T).name()] = topicName;
        return temp;
    }

    ///Example usage: ddsCallback<STATE::StateChange>(STATE::STATE_COMMAND_TOPIC_NAME,
    ///            std::bind(&ExampleReceiver::onMessage, dataReceiver, std::placeholders::_1), filterByComputer);
    ///Note that async handling is set to true in this function, while addCallback defaults to false.
    template <typename TopicType>
    bool Callback(std::string topicName, const STD_QOS::QosType qos, std::function<void(const TopicType&)> func,
        std::string filter = "", const bool& asyncHandling = true, std::string readerName = "")
    {
        std::string rName = GenerateReaderName(topicName, readerName);

        if (false == registerTopic<TopicType>(topicName, qos))
        {
            //If a topic has already been registered, this will fail but it should not stop us from continuing
        }
        if (false == createSubscriber(topicName, rName, filter))
        {
            std::stringstream sstr;
            sstr << "Failed to create subscriber for topic: " << topicName << ".";
            m_messageHandler(LogMessageType::DDS_ERROR, sstr.str());
            return false;
        }

        bool retVal = addCallback<TopicType>(topicName, rName, func, false, asyncHandling);
        if (!retVal) {
            std::stringstream sstr;
            sstr << "Failed to add callback for topic:" << topicName << ".";
            m_messageHandler(LogMessageType::DDS_ERROR, sstr.str());
        }
        m_subMap[typeid(TopicType).name()] = topicName;
        return retVal;
    }

    ///Example usage: ddsListener<STATE::StateChange, StateCommandListener>(ddsManager, STATE::STATE_COMMAND_TOPIC_NAME, filterStatus);
    template <class T, class listener>
    void Listener(std::string topicName, const STD_QOS::QosType qos,
        std::string filter = "")
    {
        Subscriber<T>(topicName, qos, filter);

        auto reader = getReader(topicName, topicName + "Reader");
        reader->set_listener(new listener, DDS::DATA_AVAILABLE_STATUS);
    }

    ///This function writes a DDS message (with ID), but first it writes m_eventID into eventID
    template <class T>
    bool WriteWID(T& message, std::string topicName = "")
    {
        //Most SCE and trainer messages have an Event ID. Most programs can simply set this once
        //and use this function to always make sure it is set.
#if defined (OPENDDW_PRECPP11)
        message.eventID = m_eventID;
#else
        message.eventID(m_eventID);
#endif
        return Write<T>(message, topicName);
    }

    ///Example usage: STATE::StateStatus closing;
    ///closing.host(_computerName);
    ///closing.app(_name);
    ///closing.state(STATE::eTrainerState::STATE_EXIT);
    ///ddsWrite<STATE::StateStatus>(closing);
    template <class T>
    bool Write(const T& message, std::string topicName = "")
    {
        std::string tName = topicName;

        if (tName.empty()) {
            decltype(m_sharedLock) lck(mutex_shr);
            auto itr = m_pubMap.find(typeid(T).name());
            if (itr == m_pubMap.end()) {
                std::stringstream sstr;
                sstr << "Trying to publish a DDS type that has no topic mapped:" << typeid(T).name() << ".";
                m_messageHandler(LogMessageType::DDS_ERROR, sstr.str());
                return false;
            }
            else {
                //l.debug("Publishing topic: %s.", typeid(T).name());
                tName = itr->second;
            }
        }
        return writeSample<T>(message, tName);
    }

    template <class T>
    bool Dispose(const T& message, std::string topicName = "")
    {
        std::string tName = topicName;

        if (tName.empty()) {
            decltype(m_sharedLock) lck(mutex_shr);
            auto itr = m_pubMap.find(typeid(T).name());
            if (itr == m_pubMap.end()) {

                std::stringstream sstr;
                sstr << "Trying to dispose a DDS type that has no topic mapped:" << typeid(T).name() << ".";
                m_messageHandler(LogMessageType::DDS_ERROR, sstr.str());
                return false;
            }
            else {
                //l.debug("Publishing topic: %s.", typeid(T).name());
                tName = itr->second;
            }
        }
        return disposeSample<T>(message, tName);
    }

    // WaitOnDiscovery's name has been changed, but we don't want to pull the rug out from people too hard.
    // Delete this function some day in the future. It was deprecated as of 16 August 2022. I suggest a delete by 16 August 2023.
    template <class T>
    [[deprecated("Calls to WaitOnDiscovery have been deprecated. Please use WaitForSubscriber<>() instead.")]]
    bool WaitOnDiscovery(int secondsToWait = 2)
    {
        std::string type_name = typeid(T).name();
        std::string message = "Calls to WaitOnDiscovery have been deprecated. Please use WaitForSubscriber<" + type_name + ">(" + std::to_string(secondsToWait) + ") instead.";
        throw std::runtime_error(message);
    }

    //Call WaitForSubscriber(0) if you have already discovered and want to see if you've lost all subscribers.
    // This function waits until it finds one (or more) Subscriber of topic T OR secondsToWait seconds expires.
    // Useful when making sure your messages are being Published on a certain topic.
    template <class T>
    bool WaitForSubscriber(std::chrono::milliseconds timeToWait = std::chrono::seconds(15))
    {
        return GetNumberOfSubscribers<T>(1, timeToWait) > 0;
    }

    //Call WaitForPublisher(0) if you have already been discovered and want to see if you've lost all connection to publishers.
    // readerName is not required unless user specifies a readerName when creating their Subscriber / Callback
    template <class T>
    bool WaitForPublisher(std::chrono::milliseconds timeToWait = std::chrono::seconds(15), std::string readerName = "")
    {
        return GetNumberOfPublishers<T>(1, timeToWait, readerName) > 0;
    }

    // Function that will wait until [max_wait] passes or until we find [min_count] number of Subscribers, whichever is faster
    template<class T>
    int GetNumberOfSubscribers(int min_count, std::chrono::milliseconds max_wait = std::chrono::seconds(15))
    {
        const std::chrono::milliseconds waitIncriment(100);
        std::chrono::milliseconds timeWaited(0);
        auto startTime = std::chrono::steady_clock::now();
        std::string topic_name = typeid(T).name();
        try {
            decltype(m_sharedLock) lck(mutex_shr);
            auto iter = m_pubMap.find(topic_name);

            std::stringstream sstr;
            if (iter == m_pubMap.end()) {
                sstr << "No Publisher found for: " << topic_name << ".";
                m_messageHandler(LogMessageType::DDS_ERROR, sstr.str());
                return false;
            }

            sstr << "Waiting a max of " << max_wait.count() << " ms for " << min_count << " Subscriber(s) of topic: " << topic_name << ".";
            m_messageHandler(LogMessageType::DDS_INFO, sstr.str());
            sstr.flush();

            std::string temp = iter->second;
            lck.unlock();
            auto dw = getWriter(temp);

            if (dw == nullptr) {
                sstr << "No writer found for: " << topic_name << ".";
                m_messageHandler(LogMessageType::DDS_ERROR, sstr.str());
                return false;
            }

            DDS::PublicationMatchedStatus pubStatus;
            pubStatus.current_count = 0;

            //Initial check for publishers
            dw->get_publication_matched_status(pubStatus);
            if (pubStatus.current_count >= min_count) {
                return pubStatus.current_count;
            }

            //Check for a publisher until out ot time
            while (timeWaited < max_wait) {
                //Sleep for the incriment time
                std::this_thread::sleep_for(waitIncriment);
                timeWaited = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime);

                dw->get_publication_matched_status(pubStatus);
                if (pubStatus.current_count >= min_count) {
                    return pubStatus.current_count;
                }
            }

            std::string addressInfo = getWriterAddress(temp);
            sstr << "Failed to find " << min_count << " on " << addressInfo << ".  Subscribers(s)... Only found " << pubStatus.current_count;
            m_messageHandler(LogMessageType::DDS_INFO, sstr.str());

            return pubStatus.current_count;
        }
        catch (...) {
            std::stringstream sstr;
            sstr << "Error thrown waiting for subscriber(s) for: " << topic_name << ".";
            m_messageHandler(LogMessageType::DDS_ERROR, sstr.str());
        }
        return 0;
    }

    template <class T>
    std::string GetSubscriberAddress()
    {
        std::string topic_name = typeid(T).name();
        try {
            std::string temp;
            {
                decltype(m_sharedLock) lck(mutex_shr);
                auto iter = m_pubMap.find(topic_name);
                if (iter == m_pubMap.end()) {
                    return std::string("Invalid Publisher for ") + topic_name;
                }

                temp = iter->second;
            }
            return getWriterAddress(temp);
        }
        catch (...) {
        }
        return std::string("Invalid Publisher for ") + topic_name;
    }

    // Function that will wait until [max_wait] passes or until we find [min_count] number of Publishers, whichever is faster
    template <class T>
    int GetNumberOfPublishers(int min_count, std::chrono::milliseconds max_wait = std::chrono::seconds(15), std::string reader_name = "")
    {
        const std::chrono::milliseconds waitIncriment(100);
        std::chrono::milliseconds timeWaited(0);
        auto startTime = std::chrono::steady_clock::now();
        std::string topic_name = typeid(T).name();

        try {
            decltype(m_sharedLock) lck(mutex_shr);
            auto iter = m_subMap.find(topic_name);

            std::stringstream sstr;
            if (iter == m_subMap.end()) {
                sstr << "No subscriber found for: " << topic_name << ".";
                m_messageHandler(LogMessageType::DDS_ERROR, sstr.str());
                return false;
            }

            sstr << "Waiting a max of " << max_wait.count() << " ms for " << min_count << " Publisher(s) of topic: " << topic_name << ".";
            m_messageHandler(LogMessageType::DDS_INFO, sstr.str());
            sstr.flush();

            std::string temp = iter->second;
            lck.unlock();

            auto genReaderName = GenerateReaderName(temp, reader_name);

            auto dr = getReader(temp, genReaderName); // Reader name == Topic name + "Reader", unless user-specified

            if (dr == nullptr) {
                sstr << "No reader found for: " << topic_name << ".";
                m_messageHandler(LogMessageType::DDS_ERROR, sstr.str());
                return false;
            }
            DDS::SubscriptionMatchedStatus subStatus;
            subStatus.current_count = 0;

            //Initial subscriber check
            dr->get_subscription_matched_status(subStatus);
            if (subStatus.current_count >= min_count) {
                return subStatus.current_count;
            }

            //Check for a subscriber until out of time
            while (timeWaited < max_wait) {
                std::this_thread::sleep_for(waitIncriment);
                timeWaited = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime);

                dr->get_subscription_matched_status(subStatus);
                if (subStatus.current_count >= min_count) {
                    return subStatus.current_count;
                }
            }

            std::string addressInfo = getReaderAddress(temp, genReaderName);
            sstr << "Failed to find " << min_count << " on " << addressInfo << ".  Publisher(s)... Only found " << subStatus.current_count;
            m_messageHandler(LogMessageType::DDS_INFO, sstr.str());

            return subStatus.current_count;
        }
        catch (...) {
            std::stringstream sstr;
            sstr << "Error thrown waiting for publisher for: " << topic_name << ".";
            m_messageHandler(LogMessageType::DDS_ERROR, sstr.str());
        }

        return 0;
    }

    template <class T>
    std::string GetPublisherAddress(std::string reader_name = "")
    {
        std::string topic_name = typeid(T).name();
        try {
            std::string temp;
            {
                decltype(m_sharedLock) lck(mutex_shr);
                auto iter = m_subMap.find(topic_name);
                if (iter == m_subMap.end()) {
                    return std::string("Invalid Subscriber for ") + topic_name;
                }
                temp = iter->second;
            }

            return getReaderAddress(temp, GenerateReaderName(temp, reader_name)); // Reader name == Topic name + "Reader", unless user-specified
        }
        catch (...) {
        }

        return std::string("Invalid subscriber for ") + topic_name;
    }

    void EventID(int id) { m_eventID = id; }

    int EventID() { return m_eventID; }

private:
    ///If you have settled on a convention where all your messages have an eventID, then you can set
    ///m_eventID once and use it for every message. If you are talking across multiple eventIDs or don't use
    ///eventID, then you should not use ddsWrite;
    int m_eventID;

    ///This publishing map is built up by calling ddsPublisher. Now you can call ddsWrite
    ///without having to specify the topicName. UNLESS you are publishing multiple topic names for the same DDS struct
    std::map<std::string, std::string> m_pubMap;

    ///The subscriber map is built when calling Callback<> or Subscriber<>. It keeps a list of all topics that the manager
    ///is subscribed to; useful when determining of there is a publisher of a given topic.
    std::map<std::string, std::string> m_subMap;

    std::shared_mutex mutex_shr;
    std::unique_lock<decltype(mutex_shr)> m_sharedLock;

    std::unique_lock<decltype(mutex_shr)> m_uniqueLock;
    // Helper function to help us maintain Subscriber & Callback functions.
    // If readername is empty, create a generic name based on topic name. Otherwise just take the specified name
    inline std::string GenerateReaderName(std::string& topicName, std::string& readerName)
    {
        return readerName.empty() ? topicName + "Reader" : readerName;
    }

}; //End of DDSSimpleManager class



//Adding and removing from arrays is not as straightforward with OpenDDS.
//Here are some template functions to help.  No longer really an issue with
//c++11.
#if defined (OPENDDW_PRECPP11)
template <typename T, typename add>
void AddToDdsArray(T& arrayInOut, add toAdd)
{
    bool foundAlready = false;
    for (unsigned int i = 0; i < arrayInOut.length(); ++i) {
        if (arrayInOut[i] == toAdd) {
            foundAlready = true;
            break;
        }
    }
    if (!foundAlready) {
        int oldLength = arrayInOut.length();
        arrayInOut.length(arrayInOut.length() + 1);
        arrayInOut[oldLength] = toAdd;
    }
}

template <typename T>
void AddToDdsArray(T& arrayInOut, std::string toAdd)
{
    bool foundAlready = false;
    for (unsigned int i = 0; i < arrayInOut.length(); ++i) {
        const std::string temp = static_cast<const char*>(arrayInOut[i]);
        if (temp == toAdd) {
            foundAlready = true;
            break;
        }
    }
    if (!foundAlready) {
        int oldLength = arrayInOut.length();
        arrayInOut.length(arrayInOut.length() + 1);
        arrayInOut[oldLength] = CORBA::string_dup(toAdd.c_str());
    }
}

template <typename T, typename add>
bool RemoveFromDdsArray(T& arrayInOut, add toRemove)
{
    bool foundAlready = false;
    for (unsigned int i = 0; i < arrayInOut.length(); ++i) {
        if (foundAlready) {
            arrayInOut[i - 1] = arrayInOut[i];
        }
        else if (arrayInOut[i] == toRemove) {
            foundAlready = true;
        }
    }
    if (foundAlready) {
        arrayInOut.length(arrayInOut.length() - 1);
    }
    return foundAlready;
}

template <typename T>
bool RemoveFromDdsArray(T& arrayInOut, std::string toRemove)
{
    bool foundAlready = false;
    for (unsigned int i = 0; i < arrayInOut.length(); ++i) {
        const std::string temp = static_cast<const char*>(arrayInOut[i]);
        if (foundAlready) {
            arrayInOut[i - 1] = arrayInOut[i];
        }
        else if (temp == toRemove) {
            foundAlready = true;
        }
    }
    if (foundAlready) {
        arrayInOut.length(arrayInOut.length() - 1);
    }
    return foundAlready;
}

//Use to compare a number/simple value against a member variable
//EX: RemoveFromDdsArray(_statusMessage.sessions, eventId, &SCE::sessionInfo::eventID);
template <typename T, typename Tcompare, typename TMember>
bool RemoveFromDdsArray(T& arrayInOut, Tcompare toRemove, TMember memberCompare)
{
    bool foundAlready = false;
    for (unsigned int i = 0; i < arrayInOut.length(); ++i) {
        if (foundAlready) {
            arrayInOut[i - 1] = arrayInOut[i];
        }
        else if (arrayInOut[i].*memberCompare == toRemove) {
            foundAlready = true;
        }
    }
    if (foundAlready) {
        arrayInOut.length(arrayInOut.length() - 1);
    }
    return foundAlready;
}
#endif
#endif

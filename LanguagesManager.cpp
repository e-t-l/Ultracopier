/** \file LanguagesManager.cpp
\brief Define the class to manage and load the languages
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QDir>
#include <QLibraryInfo>

#include "LanguagesManager.h"
#include "FacilityEngine.h"

/// \brief Create the manager and load the defaults variables
LanguagesManager::LanguagesManager()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    //load the rest
    std::vector<std::string> resourcesPaths=ResourcesManager::resourcesManager->getReadPath();
    unsigned int index=0;
    while(index<resourcesPaths.size())
    {
        std::string composedTempPath=resourcesPaths.at(index)+"Languages"+FacilityEngine::separator();
        QDir LanguagesConfiguration(QString::fromStdString(composedTempPath));
        if(LanguagesConfiguration.exists())
            languagePath.push_back(composedTempPath);
        index++;
    }
    //load the plugins
    PluginsManager::pluginsManager->lockPluginListEdition();
    connect(this,&LanguagesManager::previouslyPluginAdded,		this,	&LanguagesManager::onePluginAdded,Qt::QueuedConnection);
    connect(PluginsManager::pluginsManager,&PluginsManager::onePluginAdded,this,	&LanguagesManager::onePluginAdded,Qt::QueuedConnection);
    #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
    connect(PluginsManager::pluginsManager,&PluginsManager::onePluginWillBeRemoved,	this,	&LanguagesManager::onePluginWillBeRemoved,Qt::DirectConnection);
    #endif
    connect(PluginsManager::pluginsManager,&PluginsManager::pluginListingIsfinish,		this,	&LanguagesManager::allPluginIsLoaded,Qt::QueuedConnection);
    std::vector<PluginsAvailable> list=PluginsManager::pluginsManager->getPluginsByCategory(PluginType_Languages);
    foreach(PluginsAvailable currentPlugin,list)
        emit previouslyPluginAdded(currentPlugin);
    PluginsManager::pluginsManager->unlockPluginListEdition();
    //load the GUI option
    std::vector<std::pair<std::string, std::string> > KeysList;
    KeysList.push_back(std::pair<std::string, std::string>("Language","en"));
    KeysList.push_back(std::pair<std::string, std::string>("Language_force","false"));
    OptionEngine::optionEngine->addOptionGroup("Language",KeysList);
//	connect(this,	&LanguagesManager::newLanguageLoaded,			plugins,&PluginsManager::refreshPluginList);
//	connect(this,	&LanguagesManager::newLanguageLoaded,			this,&LanguagesManager::retranslateTheUI);
    connect(OptionEngine::optionEngine,&OptionEngine::newOptionValue,				this,	&LanguagesManager::newOptionValue,Qt::QueuedConnection);
    connect(this,	&LanguagesManager::newLanguageLoaded,			PluginsManager::pluginsManager,&PluginsManager::newLanguageLoaded,Qt::QueuedConnection);
}

/// \brief Destroy the language manager
LanguagesManager::~LanguagesManager()
{
}

/// \brief load the language selected, return the main short code like en, fr, ..
std::string LanguagesManager::getTheRightLanguage() const
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");
    if(LanguagesAvailableList.size()==0)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"empty combobox list, failing back to english");
        return "en";
    }
    else
    {
        if(!OptionEngine::optionEngine->getOptionValue("Language","Language_force").toBool())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("language auto-detection, QLocale::system().name(): ")+QLocale::system().name()+QStringLiteral(", QLocale::languageToString(QLocale::system().language()): ")+QLocale::languageToString(QLocale::system().language()));
            QString tempLanguage=getMainShortName(QLocale::languageToString(QLocale::system().language()));
            if(tempLanguage!=QStringLiteral(""))
                return tempLanguage;
            else
            {
                tempLanguage=getMainShortName(QLocale::system().name());
                if(tempLanguage!=QStringLiteral(""))
                    return tempLanguage;
                else
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("Autodetection of the language failed, QLocale::languageToString(QLocale::system().language()): ")+QLocale::languageToString(QLocale::system().language())+QStringLiteral(", QLocale::system().name(): ")+QLocale::system().name()+", failing back to english");
                    return OptionEngine::optionEngine->getOptionValue(QStringLiteral("Language"),QStringLiteral("Language")).toString();
                }
            }
        }
        else
            return OptionEngine::optionEngine->getOptionValue(QStringLiteral("Language"),QStringLiteral("Language")).toString();
    }
}

/* \brief To set the current language
\param newLanguage Should be short name code found into informations.xml of language file */
void LanguagesManager::setCurrentLanguage(const QString &newLanguage)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start: ")+newLanguage);
    //protection for re-set the same language
    if(currentLanguage==newLanguage)
        return;
    //store the language
    PluginsManager::pluginsManager->setLanguage(newLanguage);
    //unload the old language
    if(currentLanguage!=QStringLiteral("en"))
    {
        int indexTranslator=0;
        while(indexTranslator<installedTranslator.size())
        {
            QCoreApplication::removeTranslator(installedTranslator.at(indexTranslator));
            delete installedTranslator.at(indexTranslator);
            indexTranslator++;
        }
        installedTranslator.clear();
    }
    int index=0;
    while(index<LanguagesAvailableList.size())
    {
        if(LanguagesAvailableList.at(index).mainShortName==newLanguage)
        {
            //load the new language
            if(newLanguage!=QStringLiteral("en"))
            {
                QTranslator *temp;
                QStringList fileToLoad;
                //load the language main
                if(newLanguage==QStringLiteral("en"))
                    fileToLoad<<QStringLiteral(":/Languages/en/translation.qm");
                else
                    fileToLoad<<LanguagesAvailableList.at(index).path+QStringLiteral("translation.qm");
                //load the language plugin
                QList<PluginsAvailable> listLoadedPlugins=PluginsManager::pluginsManager->getPlugins();
                int indexPluginIndex=0;
                while(indexPluginIndex<listLoadedPlugins.size())
                {
                    if(listLoadedPlugins.at(indexPluginIndex).category!=PluginType_Languages)
                    {
                        QString tempPath=listLoadedPlugins.at(indexPluginIndex).path+QStringLiteral("Languages")+FacilityEngine::separator()+LanguagesAvailableList.at(index).mainShortName+FacilityEngine::separator()+QStringLiteral("translation.qm");
                        if(QFile::exists(tempPath))
                            fileToLoad<<tempPath;
                    }
                    indexPluginIndex++;
                }
                int indexTranslationFile=0;
                while(indexTranslationFile<fileToLoad.size())
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("Translation to load: ")+fileToLoad.at(indexTranslationFile));
                    temp=new QTranslator();
                    if(!temp->load(fileToLoad.at(indexTranslationFile)) || temp->isEmpty())
                    {
                        delete temp;
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("Unable to load the translation file: ")+fileToLoad.at(indexTranslationFile));
                    }
                    else
                    {
                        QCoreApplication::installTranslator(temp);
                        installedTranslator<<temp;
                    }
                    indexTranslationFile++;
                }
                temp=new QTranslator();
                if(temp->load(QString("qt_")+newLanguage, QLibraryInfo::location(QLibraryInfo::TranslationsPath)) && !temp->isEmpty())
                {
                    QCoreApplication::installTranslator(temp);
                    installedTranslator<<temp;
                }
                else
                {
                    if(!temp->load(LanguagesAvailableList.at(index).path+QStringLiteral("qt.qm")) || temp->isEmpty())
                    {
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Unable to load the translation file: qt.qm, into: "+LanguagesAvailableList.at(index).path);
                        delete temp;
                    }
                    else
                    {
                        QCoreApplication::installTranslator(temp);
                        installedTranslator<<temp;
                    }
                }
            }
            currentLanguage=newLanguage;
            FacilityEngine::facilityEngine.retranslate();
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("emit newLanguageLoaded()"));
            emit newLanguageLoaded(currentLanguage);
            return;
        }
        index++;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"unable to found language: "+newLanguage+", LanguagesAvailableList.size(): "+QString::number(LanguagesAvailableList.size()));
}

/// \brief check if short name is found into language
QString LanguagesManager::getMainShortName(const QString &shortName) const
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start"));
    int index=0;
    while(index<LanguagesAvailableList.size())
    {
        if(LanguagesAvailableList.at(index).shortName.contains(shortName) || LanguagesAvailableList.at(index).fullName.contains(shortName))
            return LanguagesAvailableList.at(index).mainShortName;
        index++;
    }
    return "";
}

/// \brief load the language in languagePath
void LanguagesManager::allPluginIsLoaded()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start"));
    setCurrentLanguage(getTheRightLanguage());
}

const QString LanguagesManager::autodetectedLanguage() const
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("language auto-detection, QLocale::system().name(): ")+QLocale::system().name()+QStringLiteral(", QLocale::languageToString(QLocale::system().language()): ")+QLocale::languageToString(QLocale::system().language()));
    QString tempLanguage=getMainShortName(QLocale::languageToString(QLocale::system().language()));
    if(tempLanguage!=QStringLiteral(""))
        return tempLanguage;
    else
    {
        tempLanguage=getMainShortName(QLocale::system().name());
        if(tempLanguage!=QStringLiteral(""))
            return tempLanguage;
    }
    return "";
}

void LanguagesManager::onePluginAdded(const PluginsAvailable &plugin)
{
    if(plugin.category!=PluginType_Languages)
        return;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start"));
    QDomElement child = plugin.categorySpecific.firstChildElement(QStringLiteral("fullName"));
    LanguagesAvailable temp;
    if(!child.isNull() && child.isElement())
        temp.fullName=child.text();
    child = plugin.categorySpecific.firstChildElement(QStringLiteral("shortName"));
    while(!child.isNull())
    {
        if(child.isElement())
        {
            if(child.hasAttribute("mainCode") && child.attribute(QStringLiteral("mainCode"))==QStringLiteral("true"))
                temp.mainShortName=child.text();
            temp.shortName<<child.text();
        }
        child = child.nextSiblingElement(QStringLiteral("shortName"));
    }
    temp.path=plugin.path;
    if(temp.fullName.isEmpty())
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"fullName empty for: "+plugin.path);
    else if(temp.path.isEmpty())
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"path empty for: "+plugin.path);
    else if(temp.mainShortName.isEmpty())
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"mainShortName empty for: "+plugin.path);
    else if(temp.shortName.size()<=0)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"temp.shortName.size()<=0 for: "+plugin.path);
    else if(!QFile::exists(temp.path+QStringLiteral("flag.png")))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"flag file not found for: "+plugin.path);
    else if(!QFile::exists(temp.path+QStringLiteral("translation.qm")) && temp.mainShortName!=QStringLiteral("en"))
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"translation not found for: "+plugin.path);
    else
        LanguagesAvailableList<<temp;
    if(PluginsManager::pluginsManager->allPluginHaveBeenLoaded())
        setCurrentLanguage(getTheRightLanguage());
}

#ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
void LanguagesManager::onePluginWillBeRemoved(const PluginsAvailable &plugin)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("start"));
    int index=0;
    while(index<LanguagesAvailableList.size())
    {
        if(plugin.path==LanguagesAvailableList.at(index).path)
        {
            return;
        }
        index++;
    }
}
#endif

void LanguagesManager::newOptionValue(const QString &group)
{
    if(group==QStringLiteral("Language"))
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("group: ")+group);
        setCurrentLanguage(getTheRightLanguage());
    }
}

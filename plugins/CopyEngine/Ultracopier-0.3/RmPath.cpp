#include "RmPath.h"

RmPath::RmPath()
{
	stopIt=false;
	waitAction=false;
	setObjectName("RmPath");
	moveToThread(this);
	start();
}

RmPath::~RmPath()
{
	stopIt=true;
	quit();
	wait();
}

void RmPath::addPath(const QString &path)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start: "+path);
	if(stopIt)
		return;
	emit internalStartAddPath(path);
}

void RmPath::skip()
{
	emit internalStartSkip();
}

void RmPath::retry()
{
	emit internalStartRetry();
}

void RmPath::run()
{
	connect(this,SIGNAL(internalStartAddPath(QString)),this,SLOT(internalAddPath(QString)));
	connect(this,SIGNAL(internalStartDoThisPath()),this,SLOT(internalDoThisPath()));
	connect(this,SIGNAL(internalStartSkip()),this,SLOT(internalSkip()));
	connect(this,SIGNAL(internalStartRetry()),this,SLOT(internalRetry()));
	exec();
}

void RmPath::internalDoThisPath()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start: "+pathList.first());
	if(!dir.rmpath(pathList.first()))
	{
		if(stopIt)
			return;
		waitAction=false;
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Unable to remove the folder: "+pathList.first());
		emit errorOnFolder(pathList.first(),tr("Unable to remove the folder"));
		return;
	}
	pathList.removeFirst();
	emit firstFolderFinish();
	checkIfCanDoTheNext();
}

void RmPath::internalAddPath(const QString &path)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start: "+path);
	pathList << path;
	checkIfCanDoTheNext();
}

void RmPath::checkIfCanDoTheNext()
{
	if(!waitAction && !stopIt && pathList.size()>0)
		emit internalStartDoThisPath();
}

void RmPath::internalSkip()
{
	waitAction=false;
	pathList.removeFirst();
	emit firstFolderFinish();
	checkIfCanDoTheNext();
}

void RmPath::internalRetry()
{
	waitAction=false;
	checkIfCanDoTheNext();
}


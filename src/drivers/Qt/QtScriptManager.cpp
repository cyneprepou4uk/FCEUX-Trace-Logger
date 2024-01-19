/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020 thor
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
// QtScriptManager.cpp
//
#ifdef __FCEU_QSCRIPT_ENABLE__
#include <stdio.h>
#include <string.h>
#include <list>

#ifdef WIN32
#include <Windows.h>
#endif

#include <QTextEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QHeaderView>
#include <QJSValueIterator>

#ifdef __QT_UI_TOOLS__
#include <QUiLoader>
#endif

#include "../../fceu.h"
#include "../../movie.h"

#include "common/os_utils.h"

#include "Qt/QtScriptManager.h"
#include "Qt/main.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleUtilities.h"
#include "Qt/ConsoleWindow.h"

//----------------------------------------------------
//----  EMU Script Object
//----------------------------------------------------
EmuScriptObject::EmuScriptObject(QObject* parent)
	: QObject(parent)
{
	script = qobject_cast<QtScriptInstance*>(parent);
}
//----------------------------------------------------
EmuScriptObject::~EmuScriptObject()
{
}
//----------------------------------------------------
void EmuScriptObject::print(const QString& msg)
{
	if (dialog != nullptr)
	{
		dialog->logOutput(msg);
	}
}
//----------------------------------------------------
void EmuScriptObject::powerOn()
{
	fceuWrapperHardReset();
}
//----------------------------------------------------
void EmuScriptObject::softReset()
{
	fceuWrapperSoftReset();
}
//----------------------------------------------------
void EmuScriptObject::pause()
{
	FCEUI_SetEmulationPaused( EMULATIONPAUSED_PAUSED );
}
//----------------------------------------------------
void EmuScriptObject::unpause()
{
	FCEUI_SetEmulationPaused(0);
}
//----------------------------------------------------
bool EmuScriptObject::paused()
{
	return FCEUI_EmulationPaused() != 0;
}
//----------------------------------------------------
int EmuScriptObject::framecount()
{
	return FCEUMOV_GetFrame();
}
//----------------------------------------------------
int EmuScriptObject::lagcount()
{
	return FCEUI_GetLagCount();
}
//----------------------------------------------------
bool EmuScriptObject::lagged()
{
	return FCEUI_GetLagged();
}
//----------------------------------------------------
void EmuScriptObject::setlagflag(bool flag)
{
	FCEUI_SetLagFlag(flag);
}
//----------------------------------------------------
bool EmuScriptObject::emulating()
{
	return (GameInfo != nullptr);
}
//----------------------------------------------------
void EmuScriptObject::message(const QString& msg)
{
	FCEU_DispMessage("%s",0, msg.toStdString().c_str());
}
//----------------------------------------------------
void EmuScriptObject::speedMode(const QString& mode)
{
	int speed = EMUSPEED_NORMAL;
	bool useTurbo = false;

	if (mode.contains("normal", Qt::CaseInsensitive))
	{
		speed = EMUSPEED_NORMAL;
	}
	else if (mode.contains("nothrottle", Qt::CaseInsensitive))
	{
		useTurbo = true;
	}
	else if (mode.contains("turbo", Qt::CaseInsensitive))
	{
		useTurbo = true;
	}
	else if (mode.contains("maximum", Qt::CaseInsensitive))
	{
		speed = EMUSPEED_FASTEST;
	}
	else
	{
		QString msg = "Invalid mode argument \"" + mode + "\" to emu.speedmode\n";
		script->throwError(QJSValue::TypeError, msg);
		return;
	}

	if (useTurbo)
	{
		FCEUD_TurboOn();
	}
	else
	{
		FCEUD_TurboOff();
	}
	FCEUD_SetEmulationSpeed(speed);
}
//----------------------------------------------------
void EmuScriptObject::registerBefore(const QJSValue& func)
{
	script->registerBefore(func);
}
//----------------------------------------------------
void EmuScriptObject::registerAfter(const QJSValue& func)
{
	script->registerAfter(func);
}
//----------------------------------------------------
void EmuScriptObject::registerStop(const QJSValue& func)
{
	script->registerStop(func);
}
//----------------------------------------------------
bool EmuScriptObject::loadRom(const QString& romPath)
{
	int ret = 0;

	if (!romPath.isEmpty())
	{
		ret = LoadGame(romPath.toLocal8Bit().constData());
	}
	return ret != 0;
}
//----------------------------------------------------
QString EmuScriptObject::getDir()
{
	return QString(fceuExecutablePath());
}
//----------------------------------------------------
//----  Qt Script Instance
//----------------------------------------------------
QtScriptInstance::QtScriptInstance(QObject* parent)
	: QObject(parent)
{
	QScriptDialog_t* win = qobject_cast<QScriptDialog_t*>(parent);

	emu = new EmuScriptObject(this);

	if (win != nullptr)
	{
		dialog = win;
		emu->setDialog(dialog);
	}
	engine = new QJSEngine(nullptr);

	emu->setEngine(engine);

	configEngine();

	QtScriptManager::getInstance()->addScriptInstance(this);
}
//----------------------------------------------------
QtScriptInstance::~QtScriptInstance()
{
	if (engine != nullptr)
	{
		engine->deleteLater();
		engine = nullptr;
	}
	QtScriptManager::getInstance()->removeScriptInstance(this);

	//printf("QtScriptInstance Destroyed\n");
}
//----------------------------------------------------
void QtScriptInstance::resetEngine()
{
	running = false;

	if (engine != nullptr)
	{
		engine->deleteLater();
		engine = nullptr;
	}
	engine = new QJSEngine(nullptr);

	if (ui_rootWidget != nullptr)
	{
		ui_rootWidget->hide();
		ui_rootWidget->deleteLater();
		ui_rootWidget = nullptr;
	}

	configEngine();
}
//----------------------------------------------------
int QtScriptInstance::configEngine()
{
	engine->installExtensions(QJSEngine::ConsoleExtension);

	QJSValue emuObject = engine->newQObject(emu);

	engine->globalObject().setProperty("emu", emuObject);

	QJSValue guiObject = engine->newQObject(this);

	engine->globalObject().setProperty("gui", guiObject);

	onFrameBeginCallback = QJSValue();
	onFrameFinishCallback = QJSValue();
	onScriptStopCallback = QJSValue();

	return 0;
}
//----------------------------------------------------
int QtScriptInstance::loadScriptFile( QString filepath )
{
	QFile scriptFile(filepath);

	running = false;

	if (!scriptFile.open(QIODevice::ReadOnly))
	{
		return -1;
	}
	QTextStream stream(&scriptFile);
	QString fileText = stream.readAll();
	scriptFile.close();

	FCEU_WRAPPER_LOCK();
	QJSValue evalResult = engine->evaluate(fileText, filepath);
	FCEU_WRAPPER_UNLOCK();

	if (evalResult.isError())
	{
		print(evalResult.toString());
		return -1;
	}
	else
	{
		running = true;
		//printf("Script Evaluation Success!\n");
	}
	//onFrameBeginCallback = engine->globalObject().property("onFrameBegin");
	//onFrameFinishCallback = engine->globalObject().property("onFrameFinish");
	//onScriptStopCallback = engine->globalObject().property("onScriptStop");

	return 0;
}
//----------------------------------------------------
void QtScriptInstance::loadObjectChildren(QJSValue& jsObject, QObject* obj)
{
	const QObjectList& childList = obj->children();

	for (auto& child : childList)
	{
		QString name = child->objectName();

		if (!name.isEmpty())
		{
			printf("Object: %s.%s\n", obj->objectName().toStdString().c_str(), child->objectName().toStdString().c_str());

			QJSValue newJsObj = engine->newQObject(child);

			jsObject.setProperty(child->objectName(), newJsObj);

			loadObjectChildren( newJsObj, child);
		}
	}
}
//----------------------------------------------------
void QtScriptInstance::loadUI(const QString& uiFilePath)
{
#ifdef __QT_UI_TOOLS__
	QFile uiFile(uiFilePath);
	QUiLoader  uiLoader;

	ui_rootWidget = uiLoader.load(&uiFile, dialog);

	if (ui_rootWidget == nullptr)
	{
		return;
	}
	
	QJSValue uiObject = engine->newQObject(ui_rootWidget);

	engine->globalObject().setProperty("ui", uiObject);

	loadObjectChildren( uiObject, ui_rootWidget);

	ui_rootWidget->show();
#else
	throwError(QJSValue::GenericError, "Error: Application was not linked against Qt UI Tools");
#endif
}
//----------------------------------------------------
void QtScriptInstance::registerBefore(const QJSValue& func)
{
	onFrameBeginCallback = func;
}
//----------------------------------------------------
void QtScriptInstance::registerAfter(const QJSValue& func)
{
	onFrameFinishCallback = func;
}
//----------------------------------------------------
void QtScriptInstance::registerStop(const QJSValue& func)
{
	onScriptStopCallback = func;
}
//----------------------------------------------------
void QtScriptInstance::print(const QString& msg)
{
	if (dialog)
	{
		dialog->logOutput(msg);
	}
}
//----------------------------------------------------
int QtScriptInstance::throwError(QJSValue::ErrorType errorType, const QString &message)
{
	running = false;
	print(message);
	engine->setInterrupted(true);
	return 0;
}
//----------------------------------------------------
void QtScriptInstance::printSymbols(QJSValue& val, int iter)
{
	int i=0;
	if (iter > 10)
	{
		return;
	}
	QJSValueIterator it(val);
	while (it.hasNext()) 
	{
		it.next();
		QJSValue child = it.value();
		qDebug() << iter << ":" << i << "  " << it.name() << ": " << child.toString();

		bool isPrototype = it.name() == "prototype";

		if (!isPrototype)
		{
			printSymbols(child, iter + 1);
		}
		i++;
	}
}
//----------------------------------------------------
int  QtScriptInstance::call(const QString& funcName, const QJSValueList& args)
{
	if (engine == nullptr)
	{
		return -1;
	}
	if (!engine->globalObject().hasProperty(funcName))
	{
		print(QString("No function exists: ") + funcName);
		return -1;
	}
	QJSValue func = engine->globalObject().property(funcName);

	FCEU_WRAPPER_LOCK();
	QJSValue callResult = func.call(args);
	FCEU_WRAPPER_UNLOCK();

	if (callResult.isError())
	{
		print(callResult.toString());
	}
	else
	{
		//printf("Script Call Success!\n");
	}

	return 0;
}
//----------------------------------------------------
void QtScriptInstance::stopRunning()
{
	if (running)
	{
		if (onScriptStopCallback.isCallable())
		{
			onScriptStopCallback.call();
		}
		running = false;
	}
}
//----------------------------------------------------
void QtScriptInstance::onFrameBegin()
{
	if (running && onFrameBeginCallback.isCallable())
	{
		onFrameBeginCallback.call();
	}
}
//----------------------------------------------------
void QtScriptInstance::onFrameFinish()
{
	if (running && onFrameFinishCallback.isCallable())
	{
		onFrameFinishCallback.call();
	}
}
//----------------------------------------------------
QString QtScriptInstance::openFileBrowser(const QString& initialPath)
{
	QString selectedFile;
	QFileDialog  dialog(this->dialog, tr("Open File") );
	QList<QUrl> urls;
	bool useNativeFileDialogVal = false;

	g_config->getOption("SDL.UseNativeFileDialog", &useNativeFileDialogVal);

	const QStringList filters({
           "Any files (*)"
         });

	urls << QUrl::fromLocalFile( QDir::rootPath() );
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).first());
	urls << QUrl::fromLocalFile( QDir( FCEUI_GetBaseDirectory() ).absolutePath() );

	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilters( filters );

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Open") );

	if (!initialPath.isEmpty() )
	{
		dialog.setDirectory( initialPath );
	}

	dialog.setOption(QFileDialog::DontUseNativeDialog, !useNativeFileDialogVal);
	dialog.setSidebarUrls(urls);

	int ret = dialog.exec();

	if ( ret )
	{
		QStringList fileList;
		fileList = dialog.selectedFiles();

		if ( fileList.size() > 0 )
		{
			selectedFile = fileList[0];
		}
	}
	return selectedFile;
}
//----------------------------------------------------
//----  Qt Script Manager
//----------------------------------------------------
QtScriptManager* QtScriptManager::_instance = nullptr;

QtScriptManager::QtScriptManager(QObject* parent)
	: QObject(parent)
{
	_instance = this;
}
//----------------------------------------------------
QtScriptManager::~QtScriptManager()
{
	_instance = nullptr;
	//printf("QtScriptManager destroyed\n");
}
//----------------------------------------------------
QtScriptManager* QtScriptManager::create(QObject* parent)
{
	QtScriptManager* mgr = new QtScriptManager(parent);

	//printf("QtScriptManager created\n");
	
	return mgr;
}
//----------------------------------------------------
void QtScriptManager::destroy(void)
{
	if (_instance != nullptr)
	{
		delete _instance;
	}
}
//----------------------------------------------------
void QtScriptManager::addScriptInstance(QtScriptInstance* script)
{
	scriptList.push_back(script);
}
//----------------------------------------------------
void QtScriptManager::removeScriptInstance(QtScriptInstance* script)
{
	auto it = scriptList.begin();

	while (it != scriptList.end())
	{
		if (*it == script)
		{
			it = scriptList.erase(it);
		}
		else
		{
			it++;
		}
	}
}
//----------------------------------------------------
void QtScriptManager::frameBeginUpdate()
{
	FCEU_WRAPPER_LOCK();
	for (auto script : scriptList)
	{
		script->onFrameBegin();
	}
	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------
void QtScriptManager::frameFinishedUpdate()
{
	FCEU_WRAPPER_LOCK();
	for (auto script : scriptList)
	{
		script->onFrameFinish();
	}
	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------
//---- Qt Script Dialog Window
//----------------------------------------------------
QScriptDialog_t::QScriptDialog_t(QWidget *parent)
	: QDialog(parent, Qt::Window)
{
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QPushButton *closeButton;
	QLabel *lbl;
	std::string filename;
	QSettings settings;

	resize(512, 512);

	setWindowTitle(tr("Qt Java Script Control"));

	mainLayout = new QVBoxLayout();

	lbl = new QLabel(tr("Script File:"));

	scriptPath = new QLineEdit();
	scriptArgs = new QLineEdit();

	g_config->getOption("SDL.LastLoadJs", &filename);

	scriptPath->setText( tr(filename.c_str()) );
	scriptPath->setClearButtonEnabled(true);
	scriptArgs->setClearButtonEnabled(true);

	jsOutput = new QTextEdit();
	jsOutput->setReadOnly(true);

	hbox = new QHBoxLayout();

	browseButton = new QPushButton(tr("Browse"));
	stopButton = new QPushButton(tr("Stop"));

	scriptInstance = new QtScriptInstance(this);

	if (scriptInstance->isRunning())
	{
		startButton = new QPushButton(tr("Restart"));
	}
	else
	{
		startButton = new QPushButton(tr("Start"));
	}

	stopButton->setEnabled(scriptInstance->isRunning());

	connect(browseButton, SIGNAL(clicked()), this, SLOT(openScriptFile(void)));
	connect(stopButton, SIGNAL(clicked()), this, SLOT(stopScript(void)));
	connect(startButton, SIGNAL(clicked()), this, SLOT(startScript(void)));

	hbox->addWidget(browseButton);
	hbox->addWidget(stopButton);
	hbox->addWidget(startButton);

	mainLayout->addWidget(lbl);
	mainLayout->addWidget(scriptPath);
	mainLayout->addLayout(hbox);

	hbox = new QHBoxLayout();
	lbl = new QLabel(tr("Arguments:"));

	hbox->addWidget(lbl);
	hbox->addWidget(scriptArgs);

	mainLayout->addLayout(hbox);

	tabWidget = new QTabWidget();
	mainLayout->addWidget(tabWidget);

	tabWidget->addTab(jsOutput, tr("Output Console"));

	propTree = new JsPropertyTree();

	propTree->setColumnCount(3);
	propTree->setSelectionMode( QAbstractItemView::SingleSelection );
	propTree->setAlternatingRowColors(true);

	auto* item = new QTreeWidgetItem();
	item->setText(0, QString::fromStdString("Name"));
	item->setText(1, QString::fromStdString("Type"));
	item->setText(2, QString::fromStdString("Value"));
	item->setTextAlignment(0, Qt::AlignLeft);
	item->setTextAlignment(1, Qt::AlignLeft);
	item->setTextAlignment(2, Qt::AlignLeft);

	propTree->setHeaderItem(item);
	propTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

	tabWidget->addTab(propTree, tr("Global Properties"));

	closeButton = new QPushButton( tr("Close") );
	closeButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
	connect(closeButton, SIGNAL(clicked(void)), this, SLOT(closeWindow(void)));

	hbox = new QHBoxLayout();
	hbox->addStretch(5);
	hbox->addWidget( closeButton, 1 );
	mainLayout->addLayout( hbox );

	setLayout(mainLayout);

	emuThreadText.reserve(4096);

	//winList.push_back(this);

	periodicTimer = new QTimer(this);

	connect(periodicTimer, &QTimer::timeout, this, &QScriptDialog_t::updatePeriodic);

	periodicTimer->start(200); // 5hz

	restoreGeometry(settings.value("QScriptWindow/geometry").toByteArray());
}
//----------------------------------------------------
QScriptDialog_t::~QScriptDialog_t(void)
{
	QSettings settings;

	//printf("Destroy JS Control Window\n");

	periodicTimer->stop();

	scriptInstance->stopRunning();

	settings.setValue("QScriptWindow/geometry", saveGeometry());
}
//----------------------------------------------------
void QScriptDialog_t::closeEvent(QCloseEvent *event)
{
	scriptInstance->stopRunning();

	//printf("JS Control Close Window Event\n");
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------
void QScriptDialog_t::closeWindow(void)
{
	scriptInstance->stopRunning();

	//printf("JS Control Close Window\n");
	done(0);
	deleteLater();
}
//----------------------------------------------------
void QScriptDialog_t::clearPropertyTree()
{
	propTree->childMap.clear();
	propTree->clear();
}
//----------------------------------------------------
void QScriptDialog_t::loadPropertyTree(QJSValue& object, JsPropertyItem* parentItem)
{
	QJSValueIterator it(object);

	while (it.hasNext())
	{
		it.next();
		QJSValue child = it.value();

		bool isPrototype = it.name() == "prototype";

		if (!isPrototype)
		{
			JsPropertyItem* item = nullptr;
			QString name = it.name();
			QString value;
			const char *type = "unknown";
			bool itemIsNew = false;

			if (parentItem == nullptr)
			{
				auto it = propTree->childMap.find(name);

				if (it != propTree->childMap.end())
				{
					item = it.value();
				}
			}
			else
			{
				auto it = parentItem->childMap.find(name);

				if (it != parentItem->childMap.end())
				{
					item = it.value();
				}
			}
			if (item == nullptr)
			{
				item = new JsPropertyItem();
				itemIsNew = true;
			}

			if (child.isArray())
			{
				type = "array";
			}
			else if (child.isBool())
			{
				type = "bool";
				value = QVariant(child.toBool()).toString();
			}
			else if (child.isCallable())
			{
				type = "function";
				value = "";
			}
			else if (child.isDate())
			{
				type = "date";
				value = child.toDateTime().toString();
			}
			else if (child.isError())
			{
				type = "error";
				value = child.toString();
			}
			else if (child.isNull())
			{
				type = "null";
				value = "null";
			}
			else if (child.isNumber())
			{
				type = "number";
				value = QVariant(child.toNumber()).toString();
			}
			else if (child.isRegExp())
			{
				type = "RegExp";
				value.clear();
			}
			else if (child.isQMetaObject())
			{
				type = "meta";
				value.clear();

				auto* meta = child.toQMetaObject();
				if (meta != nullptr)
				{
					value = meta->className();
				}
			}
			else if (child.isObject())
			{
				type = "object";
				value.clear();

				auto* obj = child.toQObject();
				if (obj != nullptr)
				{
					auto* meta = obj->metaObject();
					if (meta != nullptr)
					{
						value = meta->className();
					}
				}
			}
			else if (child.isString())
			{
				type = "string";
				value = child.toString();
			}


			if (itemIsNew)
			{
				item->setText(0, name);
				item->setText(1, type);
				item->setText(2, value);
			}
			else
			{
				bool itemHasChanged = !item->jsValue.strictlyEquals(child);

				if (itemHasChanged)
				{
					item->setText(2, value);
				}
			}
			item->jsValue = child;

			if (itemIsNew)
			{
				if (parentItem == nullptr)
				{
					propTree->addTopLevelItem(item);
					propTree->childMap[name] = item;
				}
				else
				{
					parentItem->addChild(item);
					parentItem->childMap[name] = item;
				}
			}

			if (child.isObject())
			{
				loadPropertyTree(child, item);
			}
		}
	}
}
//----------------------------------------------------
void QScriptDialog_t::updatePeriodic(void)
{
	//printf("Update JS\n");
	FCEU_WRAPPER_LOCK();
	if (!emuThreadText.isEmpty())
	{
		auto* vbar = jsOutput->verticalScrollBar();
		int vbarValue = vbar->value();
		bool vbarAtMax = vbarValue >= vbar->maximum();

		jsOutput->insertPlainText(emuThreadText);

		if (vbarAtMax)
		{
			vbar->setValue( vbar->maximum() );
		}
		emuThreadText.clear();
	}

	if (scriptInstance != nullptr)
	{
		auto* engine = scriptInstance->getEngine();

		if (engine)
		{
			QJSValue globals = engine->globalObject();

			loadPropertyTree(globals);
		}
	}
	refreshState();
	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------
void QScriptDialog_t::openJSKillMessageBox(void)
{
	int ret;
	QMessageBox msgBox(this);

	msgBox.setIcon(QMessageBox::Warning);
	msgBox.setText(tr("The JS script running has been running a long time.\nIt may have gone crazy. Kill it? (I won't ask again if you say No)\n"));
	msgBox.setStandardButtons(QMessageBox::Yes);
	msgBox.addButton(QMessageBox::No);
	msgBox.setDefaultButton(QMessageBox::No);

	ret = msgBox.exec();

	if (ret == QMessageBox::Yes)
	{
	}
}
//----------------------------------------------------
void QScriptDialog_t::openScriptFile(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string last;
	std::string dir;
	const char *exePath = nullptr;
	const char *jsPath = nullptr;
	QFileDialog dialog(this, tr("Open JS Script"));
	QList<QUrl> urls;
	QDir d;

	exePath = fceuExecutablePath();

	//urls = dialog.sidebarUrls();
	urls << QUrl::fromLocalFile(QDir::rootPath());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).first());
	urls << QUrl::fromLocalFile(QDir(FCEUI_GetBaseDirectory()).absolutePath());

	if (exePath[0] != 0)
	{
		d.setPath(QString(exePath) + "/../jsScripts");

		if (d.exists())
		{
			urls << QUrl::fromLocalFile(d.absolutePath());
		}
	}
#ifndef WIN32
	d.setPath("/usr/share/fceux/jsScripts");

	if (d.exists())
	{
		urls << QUrl::fromLocalFile(d.absolutePath());
	}
#endif

	jsPath = getenv("FCEU_QSCRIPT_PATH");

	// Parse LUA_PATH and add to urls
	if (jsPath)
	{
		int i, j;
		char stmp[2048];

		i = j = 0;
		while (jsPath[i] != 0)
		{
			if (jsPath[i] == ';')
			{
				stmp[j] = 0;

				if (j > 0)
				{
					d.setPath(stmp);

					if (d.exists())
					{
						urls << QUrl::fromLocalFile(d.absolutePath());
					}
				}
				j = 0;
			}
			else
			{
				stmp[j] = jsPath[i];
				j++;
			}
			i++;
		}

		stmp[j] = 0;

		if (j > 0)
		{
			d.setPath(stmp);

			if (d.exists())
			{
				urls << QUrl::fromLocalFile(d.absolutePath());
			}
		}
	}

	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilter(tr("JS Scripts (*.js *.JS) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter(QDir::AllEntries | QDir::AllDirs | QDir::Hidden);
	dialog.setLabelText(QFileDialog::Accept, tr("Load"));

	g_config->getOption("SDL.LastLoadJs", &last);

	if (last.size() == 0)
	{
#ifdef WIN32
		last.assign(FCEUI_GetBaseDirectory());
#else
		last.assign("/usr/share/fceux/jsScripts");
#endif
	}

	getDirFromFile(last.c_str(), dir);

	dialog.setDirectory(tr(dir.c_str()));

	// Check config option to use native file dialog or not
	g_config->getOption("SDL.UseNativeFileDialog", &useNativeFileDialogVal);

	dialog.setOption(QFileDialog::DontUseNativeDialog, !useNativeFileDialogVal);
	dialog.setSidebarUrls(urls);

	ret = dialog.exec();

	if (ret)
	{
		QStringList fileList;
		fileList = dialog.selectedFiles();

		if (fileList.size() > 0)
		{
			filename = fileList[0];
		}
	}

	if (filename.isNull())
	{
		return;
	}
	qDebug() << "selected file path : " << filename.toUtf8();

	g_config->setOption("SDL.LastLoadJs", filename.toStdString().c_str());

	scriptPath->setText(filename);

}
//----------------------------------------------------
void QScriptDialog_t::startScript(void)
{
	FCEU_WRAPPER_LOCK();
	clearPropertyTree();
	scriptInstance->resetEngine();
	if (scriptInstance->loadScriptFile(scriptPath->text()))
	{
		// Script parsing error
		FCEU_WRAPPER_UNLOCK();
		return;
	}
	// TODO add option to pass options to script main.
	QJSValue argArray = scriptInstance->getEngine()->newArray(4);
	argArray.setProperty(0, "arg1");
	argArray.setProperty(1, "arg2");
	argArray.setProperty(2, "arg3");

	QJSValueList argList = { argArray };

	scriptInstance->call("main", argList);

	refreshState();

	QJSValue globals = scriptInstance->getEngine()->globalObject();

	loadPropertyTree(globals);

	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------
void QScriptDialog_t::stopScript(void)
{
	FCEU_WRAPPER_LOCK();
	scriptInstance->stopRunning();
	refreshState();
	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------
void QScriptDialog_t::refreshState(void)
{
	if (scriptInstance->isRunning())
	{
		stopButton->setEnabled(true);
		startButton->setText(tr("Restart"));
	}
	else
	{
		stopButton->setEnabled(false);
		startButton->setText(tr("Start"));
	}
}
//----------------------------------------------------
void QScriptDialog_t::logOutput(const QString& text)
{
	if (QThread::currentThread() == consoleWindow->emulatorThread)
	{
		emuThreadText.append(text);
	}
	else
	{
		auto* vbar = jsOutput->verticalScrollBar();
		int vbarValue = vbar->value();
		bool vbarAtMax = vbarValue >= vbar->maximum();

		jsOutput->insertPlainText(text);

		if (vbarAtMax)
		{
			vbar->setValue( vbar->maximum() );
		}
	}
}
//----------------------------------------------------
#endif // __FCEU_QSCRIPT_ENABLE__

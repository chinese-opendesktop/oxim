/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you want to add, delete, or rename functions or slots, use
** Qt Designer to update this file, preserving your code.
**
** You should not define a constructor or destructor in this file.
** Instead, write your code in functions called () and destroy().
** These will automatically be called by the form's constructor and
** destructor.
*****************************************************************************/

/*
  * 某個輸入法被選擇(clicked)
  */
void OXIM_Setup::IMListView_clicked(QListViewItem *item)
{
    
    int default_IM = oxim_get_Default_IM();
    int idx = getIndexByItem(item);
    
    if (idx >= 0)
    {
	iminfo_t *imp = oxim_get_IM(idx);
	GlobolButton->setEnabled(true);
	DefaultButton->setEnabled(idx==default_IM  ? false : true);
	DeleteIM->setEnabled(imp->inuserdir ? true : false);
	if (strcasecmp(imp->modname, "gen-inp") == 0 ||
	    strcasecmp(imp->modname, "gen-inp-v1") == 0 ||
	    strcasecmp(imp->modname, "chewing") == 0)
	{
	    PropertyButton->setEnabled(true);
	}
	else
	{
	    PropertyButton->setEnabled(false);
	}

	QCheckListItem *chk = (QCheckListItem *)item;
	oxim_set_IMCircular(idx, chk->isOn());
    }
    else
    {
	GlobolButton->setEnabled(false);
	DefaultButton->setEnabled(false);
	PropertyButton->setEnabled(false);
	DeleteIM->setEnabled(false);
    }
}


int OXIM_Setup::getIndexByItem( QListViewItem *item )
{
    QListViewItemIterator it(IMListView);
    int idx = 0;
    while ( it.current() )
    {
	QCheckListItem *chk = (QCheckListItem *)it.current();
	if (chk == item)
	{
	    return idx;
	}
	++it;
	idx++;
    }
    return -1;
}


/*
  * 預設鈕被 clicked
  */
void OXIM_Setup::DefaultButton_clicked()
{
    QListViewItem *item = IMListView->selectedItem();
    if (!item)
    {
	return;
    }
    int idx = getIndexByItem(item);
    if (idx < 0)
    {
	return;
    }
    int default_IM = oxim_get_Default_IM();
    if (idx == default_IM)
    {
	return;
    }

    iminfo_t *newimp = oxim_get_IM(idx);
    oxim_set_config(DefauleInputMethod, newimp->objname);
    DefaultButton->setEnabled(false);
    item->setPixmap(1, QPixmap::fromMimeSource("cd01.png" ));
    item = getItemByIndex(default_IM);
    if (item)
    {
	item->setPixmap(1, NULL);
    }
}


QListViewItem * OXIM_Setup::getItemByIndex( int idx )
{
    QListViewItemIterator it(IMListView);
    int ret_idx = 0;
    while ( it.current() )
    {
	QListViewItem *item = (QCheckListItem *)it.current();
	if (idx == ret_idx)
	{
	    return item;
	}
	++it;
	ret_idx++;
    }
    return 0;
}

/*
  * 按下通用設定按鈕
  */
void OXIM_Setup::GlobolButton_clicked()
{
    QListViewItem *item = IMListView->selectedItem();
    if (!item)
    {
	return;
    }
    int idx = getIndexByItem(item);
    if (idx < 0)
    {
	return;
    }

    iminfo_t *imp = oxim_get_IM(idx);
    if (!imp)
    {
	return;
    }
    GlobolSetting gs ;
    gs.IMNameLabel->setText(trUtf8(imp->inpname));
    gs.AliasNameEdit->setText(trUtf8(imp->aliasname));
    if (imp->key > 0)
    {
	gs.HotKeyBox->setChecked(true);
	gs.HotKey->setEnabled(true);
	gs.HotKey->setCurrentItem(imp->key - 1);
    }
    else
    {
	gs.HotKeyBox->setChecked(false);
	gs.HotKey->setEnabled(false);
    }

    int isOK = gs.exec();
    if (!isOK)
    {
	return;
    }

    // 設定別名
    QString alias = gs.AliasNameEdit->text().stripWhiteSpace();
    if (alias.length())
    {
	item->setText(1, alias);
	oxim_set_IMAliasName(idx, alias.utf8().data());
    }
    else
    {
	item->setText(1, QString::fromUtf8(imp->inpname));
	oxim_set_IMAliasName(idx, NULL);
    }
    
    // 是否設定快速鍵
    if (!gs.HotKeyBox->isChecked())
    {
	oxim_set_IMKey(idx, -1);
	item->setText(2, trUtf8("無"));
    }
    else
    {
	int key = gs.HotKey->currentItem();
	
	if (key == 9)
	{
	    key = 10;
	}
	else
	{
	    key ++;
	}
	
	if (oxim_set_IMKey(idx, key))
	{
	    item->setText(2, QString("Ctrl+Alt+") + gs.HotKey->currentText());
	}
	else
	{
	    QMessageBox::information( this, trUtf8("快速鍵重複"),
	      trUtf8("您指定的快速鍵，已經被其他輸入法使用了！\n") +
              trUtf8("請重新指定另一組快速鍵。") );
	}
    }
}

#include <X11/Xlib.h>
/*
 * 重新寫入 config 檔案
 */
void OXIM_Setup::UpdateConfig()
{
    int size;
    
    // 變更字型名稱
    oxim_set_config(DefaultFontName, FontName->currentText().utf8().data());

    // 讀取組字區字型大小
    size = PreeditSize->value();
    oxim_set_config(PreeditFontSize, QString::number(size).utf8().data());

    // 設定選字區字型大小
    size = SelectSize->value();
    oxim_set_config(SelectFontSize, QString::number(size).utf8().data());

    // 設定狀態區字型大小
    size = StatusSize->value();
    oxim_set_config(StatusFontSize, QString::number(size).utf8().data());

    // 設定選單區字型大小
    size = MenuSize->value();
    oxim_set_config(MenuFontSize, QString::number(size).utf8().data());

    // 設定符號表字型大小
    size = SymbolSize->value();
    oxim_set_config(SymbolFontSize, QString::number(size).utf8().data());

    // 設定 XCIN 風格字型大小
    size = XCINSize->value();
    oxim_set_config(XcinFontSize, QString::number(size).utf8().data());

    // 重新寫入 config 檔案
    oxim_make_config();
    
    // 寫入片語表
    QString qpharse_name(QString::fromUtf8(oxim_user_dir()).append("/tables/default.phr"));
    QFile qpharse_file(qpharse_name);
    if (qpharse_file.open(IO_WriteOnly))
    {
	for (int i=0 ; i < PharseTable->numRows() ; i++)
	{
	    QString label = PharseTable->verticalHeader()->label(i);
	    QString pharse = PharseTable->text(i, 0).stripWhiteSpace();
	    QTextStream stream(&qpharse_file);
	    stream << label << " " << "\"" << pharse << "\"\n";
	}
	qpharse_file.close();
    }

    // 取得 OXIM 控制視窗 atom
    Display *dpy = x11AppDisplay();
    Atom oxim_atom = XInternAtom(dpy, OXIM_ATOM, true);
    // 有 oxim_atom 表示 OXIM 執行中，送 ClientMessage 給 OXIM
    // 叫他 Reload
    if (oxim_atom != None)
    {
	Window config_win = XGetSelectionOwner(dpy, oxim_atom);
	if (config_win != None)
	{
	    XClientMessageEvent event;
	    event.type = ClientMessage;
	    event.window = config_win;
	    event.message_type = oxim_atom;
	    event.format = 32;
	    event.data.l[0] = OXIM_CMD_RELOAD;
	    XSendEvent(dpy, config_win, False, 0, (XEvent *)&event);
	}
    }

    QDialog::accept();
}


/*
  * 初始化輸入法列表
  */
void OXIM_Setup::IMListInit( void )
{
    QCheckListItem *item = NULL;

    oxim_init();
    VersionLabel->setText(tr("Open X Input Method ") + QString(oxim_version()));
    DefaultButton->setIconSet(QIconSet::QIconSet(QPixmap::fromMimeSource("cd01.png")));
    AddIM->setIconSet(QIconSet::QIconSet(QPixmap::fromMimeSource("edit_add.png")));
    DeleteIM->setIconSet(QIconSet::QIconSet(QPixmap::fromMimeSource("edit_remove.png")));
    IMListView->clear();
    IMListView->setSorting(-1);

    int n_IM = oxim_get_NumberOfIM();
    int default_idx = oxim_get_Default_IM();
    for (int i=0 ; i < n_IM ; i++)
    {
	char key[2];
	key[1] = '\0';
	iminfo_t *imp = oxim_get_IM(i);
	item = new QCheckListItem(IMListView, (i ? item : 0), "", QCheckListItem::CheckBox);
	
	// 輸入法名稱
	item->setText(1, QString::fromUtf8(imp->aliasname ? imp->aliasname : imp->inpname));
	// 快速鍵
	if (imp->key > 0 && imp->key <= 10)
	{
	    if (imp->key == 10)
	    {
		key[0] = '0';
	    }
	    else
	    {
		key[0] = '0' + (char)imp->key;
	    }
	    item->setText(2, QString("Ctrl+Alt+") + QString(key));
	}
	else
	{
	    item->setText(2, QString::fromUtf8("無"));
	}
	
	if (i == default_idx)
	{
	    item->setPixmap(1, QPixmap::fromMimeSource("cd01.png" ));
	}

	item->setOn(imp->circular);
	item->setText(3, (imp->inuserdir ? "" : trUtf8("<內建>")));
	
	settings_t *settings = oxim_get_im_settings(imp->objname);
	if (settings)
	{
	    oxim_set_IMSettings(i, settings);
	    oxim_settings_destroy(settings);
	}
    }
    // 讀取系統的字型資料
    bool ok;
    QString size;
    QFontDatabase fdb;
    FontName->clear();
    FontName->insertStringList(fdb.families());

    // 讀取 OXIM 預設的字型名稱
    FontName->setEditText(QString::fromUtf8(oxim_get_config(DefaultFontName)));

    // 讀取組字區字型大小
    size = QString::fromUtf8(oxim_get_config(PreeditFontSize));
    PreeditSize->setValue(size.toInt(&ok, 10));

    // 讀取選字區字型大小
    size = QString::fromUtf8(oxim_get_config(SelectFontSize));
    SelectSize->setValue(size.toInt(&ok, 10));

    // 讀取狀態區字型大小
    size = QString::fromUtf8(oxim_get_config(StatusFontSize));
    StatusSize->setValue(size.toInt(&ok, 10));

    // 讀取選單區字型大小
    size = QString::fromUtf8(oxim_get_config(MenuFontSize));
    MenuSize->setValue(size.toInt(&ok, 10));

    // 讀取符號表字型大小
    size = QString::fromUtf8(oxim_get_config(SymbolFontSize));
    SymbolSize->setValue(size.toInt(&ok, 10));

    // 讀取 XCIN 風格字型大小
    size = QString::fromUtf8(oxim_get_config(XcinFontSize));
    XCINSize->setValue(size.toInt(&ok, 10));
    
    // 
    int numRow = PharseTable->numRows();
    for (int i=0 ; i < numRow ; i++)
    {
	PharseTable->removeRow(0);
    }

    // 設定片語表
    numRow = 0;
    for (int i=0 ; i < 50 ; i++)
    {
	int key = oxim_code2key(i);
	char chrkey[2];
	chrkey[1] = '\0';
	if (key)
	{
	    PharseTable->insertRows(numRow);
	    chrkey[0] = (char)key;
	    QString pharse(trUtf8(oxim_qphrase_str(key)));
	    PharseTable->verticalHeader()->setLabel(numRow, chrkey);
	    PharseTable->setText(numRow, 0, pharse.stripWhiteSpace());
	    numRow ++;
	}
    }
}

/*
  * 按下屬性按鈕
  */
void OXIM_Setup::PropertyButton_clicked()
{
    QListViewItem *item = IMListView->selectedItem();
    if (!item)
    {
	return;
    }
    int idx = getIndexByItem(item);
    if (idx < 0)
    {
	return;
    }

    iminfo_t *imp = oxim_get_IM(idx);
    if (!imp)
    {
	return;
    }

    QString modname(imp->modname);
    if (modname == "gen-inp" || modname == "gen-inp-v1")
    {
	RunGenCinForm(idx, imp);
    }
    else if (modname == "chewing")
    {
	RunChewing(idx, imp);
    }
}


void OXIM_Setup::RunGenCinForm( int idx, iminfo_t *imp )
{
    GenCin form;
    settings_t *settings = NULL;
    
    form.IMName->setText(imp->aliasname ? trUtf8(imp->aliasname) : trUtf8(imp->inpname));
    form.imname = imp->objname;

    if (imp->settings)
    {
	settings = imp->settings;
	int yesno;
	// AutoCompose
	if (oxim_setting_GetBoolean(settings, "AutoCompose", &yesno))
	{
	    form.AutoCompose->setChecked(yesno);
	}
	
	// AutoUpChar
	if (oxim_setting_GetBoolean(settings, "AutoUpChar", &yesno))
	{
	    form.AutoUpChar->setChecked(yesno);
	}
	
	// AutoFullUp
	if (oxim_setting_GetBoolean(settings, "AutoFullUp", &yesno))
	{
	    form.AutoFullUp->setChecked(yesno);
	}
	
	// SpaceAutoUp
	if (oxim_setting_GetBoolean(settings, "SpaceAutoUp", &yesno))
	{
	    form.SpaceAutoUp->setChecked(yesno);
	}
	
	// SelectKeyShift
	if (oxim_setting_GetBoolean(settings, "SelectKeyShift", &yesno))
	{
	    form.SelectKeyShift->setChecked(yesno);
	}
	
	// SpaceIgnore
	if (oxim_setting_GetBoolean(settings, "SpaceIgnore", &yesno))
	{
	    if (yesno)
		form.SpaceIgnoreYes->setChecked(true);
	    else
		form.SpaceIgnoreNo->setChecked(true);
	}

	// SpaceReset
	if (oxim_setting_GetBoolean(settings, "SpaceReset", &yesno))
	{
	    form.SpaceReset->setChecked(yesno);
	}
	
	// WildEnable
	if (oxim_setting_GetBoolean(settings, "WildEnable", &yesno))
	{
	    form.WildEnable->setChecked(yesno);
	}
	
	// 設定 EndKey
	if (oxim_setting_GetBoolean(settings, "EndKey", &yesno))
	{
	    form.EndKey->setChecked(yesno);
	}

	// 設定特殊字根
	char *spstroke;
	if (oxim_setting_GetString(settings, "DisableSelectList", &spstroke))
	{
	    if (strcasecmp(spstroke, "None") != 0)
		form.DisableSelectList->setText(spstroke);
	}
    }

    int isOK = form.exec();

    if (!isOK) //  按下取消或直接關掉視窗
    {
	return;
    }

    settings = oxim_settings_create();
    // AutoCompose
    oxim_settings_add_key_value(settings, "AutoCompose",
	form.AutoCompose->isChecked() ? "Yes" : "No");
    // AutoUpChar
    oxim_settings_add_key_value(settings, "AutoUpChar",
	form.AutoUpChar->isChecked() ? "Yes" : "No");
    // AutoFullUp
    oxim_settings_add_key_value(settings, "AutoFullUp",
	form.AutoFullUp->isChecked() ? "Yes" : "No");
    // SpaceAutoUp
    oxim_settings_add_key_value(settings, "SpaceAutoUp",
	form.SpaceAutoUp->isChecked() ? "Yes" : "No");
    // SelectKeyShift
    oxim_settings_add_key_value(settings, "SelectKeyShift",
	form.SelectKeyShift->isChecked() ? "Yes" : "No");
    // SpaceIgnore
    oxim_settings_add_key_value(settings, "SpaceIgnore",
	form.SpaceIgnoreYes->isChecked() ? "Yes" : "No");
    // SpaceReset
    oxim_settings_add_key_value(settings, "SpaceReset",
	form.SpaceReset->isChecked() ? "Yes" : "No");
    // WildEnable
    oxim_settings_add_key_value(settings, "WildEnable",
	form.WildEnable->isChecked() ? "Yes" : "No");
    // EndKey
    oxim_settings_add_key_value(settings, "EndKey",
	form.EndKey->isChecked() ? "Yes" : "No");
    // DisableSelectList
    QString dstroke = form.DisableSelectList->text().stripWhiteSpace();
    oxim_settings_add_key_value(settings, "DisableSelectList",
	dstroke.length() ? dstroke.utf8().data() : "None");
    
    oxim_set_IMSettings(idx, settings);
    oxim_settings_destroy(settings);
}

/*
  * 新酷音輸入法屬性
  */
void OXIM_Setup::RunChewing( int idx, iminfo_t *imp )
{
    Chewing form;
    settings_t *settings = NULL;
    
    if (imp->settings)
    {
	settings = imp->settings;
	int number;
	if (oxim_setting_GetInteger(settings, "KeyMap", &number))
	{
	    switch (number)
	    {
	    case 1:
		form.KeyMap1->setChecked(true);
		break;
	    case 2:
		form.KeyMap2->setChecked(true);
		break;
	    case 3:
		form.KeyMap3->setChecked(true);
		break;
	    case 4:
		form.KeyMap4->setChecked(true);
		break;
	    case 5:
		form.KeyMap5->setChecked(true);
		break;
	    case 6:
		form.KeyMap6->setChecked(true);
		break;
	    case 7:
		form.KeyMap7->setChecked(true);
		break;
	    case 8:
		form.KeyMap8->setChecked(true);
		break;
	    default:
		form.KeyMap0->setChecked(true);
	    }
	}
	//
	if (oxim_setting_GetInteger(settings, "SelectionKeys", &number))
	{
	    switch (number)
	    {
	    case 1:
		form.SelectionKeys1->setChecked(true);
		break;
	    case 2:
		form.SelectionKeys2->setChecked(true);
		break;
	    case 3:
		form.SelectionKeys3->setChecked(true);
		break;
	    default:
		form.SelectionKeys0->setChecked(true);
	    }
	}
	//
	if (oxim_setting_GetInteger(settings, "CapsLockMode", &number))
	{
	    if (number != 0)
		form.CapsLockMode1->setChecked(true);
	}
    }

    int isOk = form.exec();
    
    if (!isOk)
	return;

    settings = oxim_settings_create();
    if (form.KeyMap0->isChecked())
	oxim_settings_add_key_value(settings, "KeyMap", "0");
    else if (form.KeyMap1->isChecked())
	oxim_settings_add_key_value(settings, "KeyMap", "1");
    else if (form.KeyMap2->isChecked())
	oxim_settings_add_key_value(settings, "KeyMap", "2");
    else if (form.KeyMap3->isChecked())
	oxim_settings_add_key_value(settings, "KeyMap", "3");
    else if (form.KeyMap4->isChecked())
	oxim_settings_add_key_value(settings, "KeyMap", "4");
    else if (form.KeyMap5->isChecked())
	oxim_settings_add_key_value(settings, "KeyMap", "5");
    else if (form.KeyMap6->isChecked())
	oxim_settings_add_key_value(settings, "KeyMap", "6");
    else if (form.KeyMap7->isChecked())
	oxim_settings_add_key_value(settings, "KeyMap", "7");
    else if (form.KeyMap8->isChecked())
	oxim_settings_add_key_value(settings, "KeyMap", "8");
    
    if (form.SelectionKeys0->isChecked())
	oxim_settings_add_key_value(settings, "SelectionKeys", "0");
    else if (form.SelectionKeys1->isChecked())
	oxim_settings_add_key_value(settings, "SelectionKeys", "1");
    else if (form.SelectionKeys2->isChecked())
	oxim_settings_add_key_value(settings, "SelectionKeys", "2");
    else if (form.SelectionKeys3->isChecked())
	oxim_settings_add_key_value(settings, "SelectionKeys", "3");

    if (form.CapsLockMode0->isChecked())
	oxim_settings_add_key_value(settings, "CapsLockMode", "0");
    else
	oxim_settings_add_key_value(settings, "CapsLockMode", "1");

    oxim_set_IMSettings(idx, settings);
    oxim_settings_destroy(settings);
}


/*
  * 安裝輸入法
  */
void OXIM_Setup::AddIM_clicked()
{
    QString tables_dir(QString::fromUtf8(oxim_user_dir()).append("/tables"));
    QString modules_dir(QString::fromUtf8(oxim_user_dir()).append("/modules"));
    bool isReload = false;

    InstallIM form;
    int isOk = form.exec();
    
    if (!isOk)
	return;
    
    QMap<QCheckListItem *, QString>::Iterator it;
    for (it = form.CheckMap.begin(); it != form.CheckMap.end(); ++it)
    {
	QCheckListItem *item = it.key();
	QString cinfile = QString::fromUtf8(oxim_user_dir()) + "/" + it.data();
	if (item->isOn())
	{
	    convert2table(cinfile);
	    QFile file(cinfile);
	    file.remove();
	    isReload = true;
	}
    }

    // 是否有選擇檔案
    if (form.FileName->text().stripWhiteSpace().length())
    {
	// 檢查檔案是否存在
	QFileInfo file(form.FileName->text().stripWhiteSpace());
	if (file.exists())
	{
	    QString path(file.dirPath()); // 原始檔路徑
	    QString name(file.fileName()); // 原始檔檔名
	    QString basename = file.baseName(FALSE); // 不含副檔名的檔名
	    QString extname = file.extension(FALSE); // 副檔名
	    printf("%s\n", extname.utf8().data());
	    // 檢查檔案的擴充名稱(*.cin、*.tab、*.so)
	    if (extname == "cin" || extname == "gz" || extname == "in")
	    {
		QString tabname(basename.append(".tab"));
		QString cmd( "oxim2tab" );
		cmd.append( " -o " );
		cmd.append(tables_dir + "/" + tabname);
		cmd.append( " " );
		cmd.append(form.FileName->text());
		
		if (system(cmd.utf8().data()) == 0)
		{
		    if(oxim_CheckTable(tables_dir.utf8().data(), tabname.utf8().data(), NULL, NULL))
		    {
			isReload = true;
			printf("轉檔成功\n");
		    }
		    else
		    {
			printf("1.輸入法轉檔失敗\n");
		    }
		}
		else
		{
		    printf("2.輸入法轉檔失敗\n");
		}
	    }
	    else if (extname == "tab")
	    {
		// 檢查 tab 檔是否合法
		if (oxim_CheckTable(path.utf8().data(),
				    name.utf8().data(),
				    NULL, NULL))
		{
		    QString source(form.FileName->text().stripWhiteSpace());
		    QString target(tables_dir + "/" + basename+".tab");
		    if (CopyBinaryFile(target.utf8().data(), source.utf8().data()))
		    {
			isReload = true;
			printf("安裝成功\n");
		    }
		    else
		    {
			printf("安裝失敗\n");
		    }
		}
		else
		{
		    printf("不支援此種格式的 tab 檔案！\n");
		}
	    }
	    else
		printf("不是 OXIM 支援的輸入法檔案\n");
	}
    }
    if (isReload)
    {
	// 重新寫入 config 檔案
	oxim_make_config();
	// 清除 oxim config
	oxim_destroy();
	// 重新讀取
	IMListInit();
    }
}


bool OXIM_Setup::CopyBinaryFile( QString target, QString source )
{
    QFile sourceFile(source);
    QFile targetFile(target);
    Q_LONG readBytes;
    char data[10240];  // 資料存取區
    if (!sourceFile.open(IO_ReadOnly)) // 唯讀開啟
    {
	return false;
    }
    if (!targetFile.open(IO_WriteOnly|IO_Truncate)) // 寫入
    {
	return false;
    }
     while ((readBytes = sourceFile.readBlock(data, sizeof(data))) > 0)
    {
	 targetFile.writeBlock(data, readBytes);
    }
     sourceFile.close();
     targetFile.close();
     return true;
}


void OXIM_Setup::DeleteIM_clicked()
{
    QListViewItem *item = IMListView->selectedItem();
    if (!item)
    {
	return;
    }
    int idx = getIndexByItem(item);
    if (idx < 0)
    {
	return;
    }

    iminfo_t *imp = oxim_get_IM(idx);
    if (!imp)
    {
	return;
    }

    // 不在使用者目錄下的不能殺
    if (!imp->inuserdir)
	return;

    // 不是 tab 檔，不能殺
    if (strcasecmp(imp->modname, "gen-inp") != 0 && strcasecmp(imp->modname, "gen-inp-v1") != 0)
    {
	return;
    }

    bool yesno = QMessageBox::warning( this, trUtf8("移除輸入法"),
	      trUtf8("這樣會將您選定的輸入法從您的電腦中移除，\n") +
	      trUtf8("無法再繼續使用，真的要這樣做嗎？"),
              trUtf8("確定移除(&Y)"), trUtf8("取消(&N)"), 0, 1, 0 );
    
    if (yesno != 0)
	return;

    QString tables_dir(QString::fromUtf8(oxim_user_dir()).append("/tables"));
    QString tab_name(QString::fromUtf8(imp->objname).append(".tab"));
    QFile::remove(tables_dir.append("/").append(tab_name));
    
    // 重新寫入 config 檔案
    oxim_make_config();
    // 清除 oxim config
    oxim_destroy();
    // 重新讀取
    IMListInit();
}


/*
  * 將參考檔轉檔到使用者目錄下
  */
void OXIM_Setup::convert2table( QString sourceFile )
{
    QFileInfo fileInfo(sourceFile);
    QString tables_dir(QString::fromUtf8(oxim_user_dir()).append("/tables"));
    QString targetFile = tables_dir.append("/") + fileInfo.baseName().append(".tab");

    printf("source name = %s \n", sourceFile.utf8().data());
    printf("target name = %s \n", targetFile.utf8().data());
    
    QString cmd = QString( "oxim2tab" ) + " -o " + targetFile + " " + sourceFile;
    system(cmd.utf8().data());
}

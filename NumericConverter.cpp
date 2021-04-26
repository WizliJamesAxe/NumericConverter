#include "NumericConverter.hpp"

void WINAPI GetGlobalInfoW(struct GlobalInfo *Info)
{
	Info->StructSize=sizeof(struct GlobalInfo);
	Info->MinFarVersion=FARMANAGERVERSION;
	Info->Version=PLUGIN_VERSION;
	Info->Guid=MainGuid;
	Info->Title=PLUGIN_NAME;
	Info->Description=PLUGIN_DESC;
	Info->Author=PLUGIN_AUTHOR;
}

/*
	Функция SetStartupInfoW вызывается один раз, перед всеми
	другими функциями. Она передается плагину информацию,
	необходимую для дальнейшей работы.
*/
void WINAPI SetStartupInfoW(const struct PluginStartupInfo* psi)
{
	if (psi->StructSize >= sizeof(PluginStartupInfo))
	{
		Info = *psi;
		FSF = *psi->FSF;
		Info.FSF = &FSF;
	}
}

// Получение строки из языкового файла
const wchar_t* GetMsg(int MsgId)
{
	return Info.GetMsg(&MainGuid, MsgId);
}

// Получение FARом информации о плагине
void WINAPI GetPluginInfoW(struct PluginInfo* Info)
{
	Info->StructSize = sizeof(*Info);

	// Отображать в плагинах
	Info->Flags = PF_NONE;

	// Заполняем PluginMenu
	static const wchar_t* PluginMenuStrings[1];
	PluginMenuStrings[0] = GetMsg(MTitle);

	Info->PluginMenu.Guids = &MenuGuid;
	Info->PluginMenu.Strings = PluginMenuStrings;
	Info->PluginMenu.Count = ARRAYSIZE(PluginMenuStrings);
}


// Вызывается при создании новой копии плагина
HANDLE WINAPI OpenW(const struct OpenInfo* OInfo)
{
	// Если произошла отмена диалога то выходим
	if (!ShowDialog())
		return NULL;
	
	// Проверка количества на 0
	if (NumbersInLine == 0)
	{
		const wchar_t* MsgItems1[] = { GetMsg(MTitle), GetMsg(MTextZeroCount) };
		Info.Message(&MainGuid,	NULL, FMSG_WARNING | FMSG_MB_OK, L"Contents", MsgItems1, ARRAYSIZE(MsgItems1), 0);

		return NULL;
	}

	// Проверка Суффикса
	if (lstrlenW(suffix) == 0)
	{
		const wchar_t* MsgItems2[] = { GetMsg(MTitle), GetMsg(MTextEmptySuffix) };
		Info.Message(&MainGuid, NULL, FMSG_WARNING | FMSG_MB_OK, L"Contents", MsgItems2, ARRAYSIZE(MsgItems2), 0);

		return NULL;
	}

	// Останавливаем Экран
	HANDLE hScreen = Info.SaveScreen(0, 0, -1, -1);
	
	// Выводим сообщение о конвертации
	const wchar_t* MsgItems[] = { GetMsg(MTitle), GetMsg(MTextConverting) };
	Info.Message(&MainGuid, nullptr, 0, NULL, MsgItems, ARRAYSIZE(MsgItems), 0);


	// Получаем информацию о панели в PInfo
	struct PanelInfo PInfo = { sizeof(PanelInfo) };
	Info.PanelControl(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, &PInfo);

	// Получаем размер текущего каталога панели
	int Size = (int)Info.PanelControl(PANEL_ACTIVE, FCTL_GETPANELDIRECTORY, 0, 0);
	FarPanelDirectory* dirInfo = (FarPanelDirectory*)malloc(Size);

	// Имя файла для отображателя
	wchar_t* fileName = NULL;

	if (dirInfo)
	{
		// Получаем текущий каталог панели
		dirInfo->StructSize = sizeof(FarPanelDirectory);
		Info.PanelControl(PANEL_ACTIVE, FCTL_GETPANELDIRECTORY, Size, dirInfo);

		// Идём по всем выделенным элементам
		for (size_t I = 0; I < PInfo.SelectedItemsNumber; I++)
		{

			// Получаем выделенный элемент
			size_t Size = Info.PanelControl(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, I, 0);
			PluginPanelItem* PPI = (PluginPanelItem*)malloc(Size);

			if (PPI)
			{
				FarGetPluginPanelItem gpi = { sizeof(FarGetPluginPanelItem), Size, PPI };
				Info.PanelControl(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, I, &gpi);

				size_t FullNameLen = (size_t)lstrlenW(dirInfo->Name) + lstrlenW(PPI->FileName) + 8;
				wchar_t* FullName = new wchar_t[FullNameLen];
				wchar_t* FullNameOut = new wchar_t[FullNameLen + lstrlenW(suffix)];
				if (FullName)
				{
					lstrcpy(FullName, dirInfo->Name);
					FSF.AddEndSlash(FullName);
					lstrcat(FullName, PPI->FileName);

					lstrcpy(FullNameOut, dirInfo->Name);
					FSF.AddEndSlash(FullNameOut);
					lstrcat(FullNameOut, PPI->FileName);
					lstrcat(FullNameOut, suffix);

					// Обрабатываем Файл
					if (ConvertFile(FullName, FullNameOut))
						return NULL;

					delete[] FullName;
					if (I == 0)
						fileName = FullNameOut;
					else
						delete[] FullNameOut;
				}					
				
				free(PPI);
			}
		}
		free(dirInfo);
	}

	// Восстанавливаем экран
	Info.RestoreScreen(hScreen);

	// Обновляем панель
	Info.PanelControl(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, 0);
	Info.PanelControl(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, 0);


	// Открываем отображатель
	if (fileName != NULL)
	{
		Info.Viewer(fileName, fileName, 0, 0, -1, -1, VF_NONE, CP_DEFAULT);
	}
	return NULL;
}

//Вывод сообщения
void ShowMessage(const wchar_t* Mes, FARMESSAGEFLAGS flags)
{

	const wchar_t* MsgItems[] =
	{
		GetMsg(MTitle),
		Mes
	};

	Info.Message(
		&MainGuid,				// GUID 
		NULL,
		flags | FMSG_MB_OK,		// Flags 
		L"Contents",			// HelpTopic 
		MsgItems,				// Items 
		ARRAYSIZE(MsgItems),	// ItemsNumber 
		0);						// ButtonsNumber 

}

// Отображаем диалог
bool ShowDialog()
{
	// Создаем диалог
	PluginDialogBuilder Builder(Info, MainGuid, DialogGuid, MTitle, L"Contents", NULL);
	
	// Делим диалог на колонки и формируем первую
	Builder.StartColumns();

	// Выберите тип
	Builder.AddText(MTextType);

	// Список типов
	const int TypeIDs[] = { MTypeInt16, MTypeInt32, MTypeDouble, MTypeInt8, MTypeInt64, MTypeFloat};
	Builder.AddRadioButtons(&ConvertMode, ARRAYSIZE(TypeIDs), TypeIDs, true);
	
	// Вторая колонка
	Builder.ColumnBreak();

	// Выберите формат
	Builder.AddText(MTextFormat);

	// Список форматов
	const int FormatIDs[] = { MTypeSigned, MTypeUnsigned};
	Builder.AddRadioButtons(&ConvertFormat, ARRAYSIZE(FormatIDs), FormatIDs, true);
	
	// Закрываем колонки
	Builder.EndColumns();

	Builder.AddSeparator();

	// Количество числе в строке
	Builder.AddText(MTextCountInLine);

	Builder.AddUIntEditField(&NumbersInLine, 8);

	Builder.AddSeparator();

	// Суффикс выходного файла
	Builder.AddText(MTextSuffix);

	Builder.AddEditField((wchar_t*)&suffix, 40, 40);

	// Кнопки
	Builder.AddOKCancel(MButtonGO, MButtonCancel);

	// Показываем диалог
	return Builder.ShowDialog();
}

// Функция преобразования
errno_t ConvertFile(const wchar_t* FileNameIN, const wchar_t* FileNameOUT)
{
	// Проверка валидности параметров конвертации
	if (!((ConvertMode >= 0 && ConvertMode < EnumMode::LastMode) && 
		(ConvertFormat >= 0 && ConvertFormat < EnumFormat::LastFormat)))
		return 1;
	

	errno_t err;
	FILE* file_in;
	FILE* file_out;

	// Открываем файлы
	err = _wfopen_s(&file_in, FileNameIN, L"rb");
	if (err != 0)
		return 1;

	err = _wfopen_s(&file_out, FileNameOUT, L"wt");
	if (err != 0)
		return 2;


	const size_t buff_size = 512;
	char buffer[buff_size];
	size_t read_count = 0;

	std::stringstream output;

	NumbersInLineNow = 0;
	if (file_in && file_out)
		while (!feof(file_in))
		{
			// Считываем в буффер
			read_count = fread_s(buffer, buff_size, 1, buff_size, file_in);

			// Выполняем преобразование
			switch (ConvertMode)
			{
				// Для Инта
			case EnumMode::Int:
				if (ConvertFormat)
					buff_to_string<uint32_t>(buffer, read_count, output);
				else
					buff_to_string<int32_t>(buffer, read_count, output);
				break;

				// Для Дабла
			case EnumMode::Double:
				buff_to_string<double>(buffer, read_count, output);
				break;

				// Для Шорта
			case EnumMode::Short:
				if (ConvertFormat)
					buff_to_string<unsigned short>(buffer, read_count, output);
				else
					buff_to_string<short>(buffer, read_count, output);
				break;

				// Для Чара
			case EnumMode::Char:
				if (ConvertFormat)
					buff_to_string<uint8_t>(buffer, read_count, output);
				else
					buff_to_string<int8_t>(buffer, read_count, output);
				break;

				// Для Лонга
			case EnumMode::Long:
				if (ConvertFormat)
					buff_to_string<uint64_t>(buffer, read_count, output);
				else
					buff_to_string<int64_t>(buffer, read_count, output);
				break;

				// Для Флоата
			case EnumMode::Float:
				buff_to_string<float>(buffer, read_count, output);
				break;
			}

			// Записываем
			fwrite(output.str().c_str(), 1, output.str().length(), file_out);

			output.str("");
		}

	// Закрываем файлы
	if (file_in)
		err = fclose(file_in);
	if (file_out)
		err = fclose(file_out);

	// Выходим из функции
	return 0;
}

size_t get_number_size()
{
	switch (ConvertMode)
	{
	case EnumMode::Char:
		return 1;
	case EnumMode::Short:
	case EnumMode::Float:
		return 2;
	case EnumMode::Int:
		return 4;
	case EnumMode::Long:
	case EnumMode::Double:
		return 8;
	default:
		return 0;
	}
}


// scientific << setprecision(15)
template<typename Type>
errno_t buff_to_string(char* buffer, size_t buff_size, std::stringstream& output)
{
	size_t num_size = get_number_size();
	if (num_size == 0)
		return 1;

	buff_size -= (buff_size % num_size);

	Type* val = (Type*)buffer;
	for (size_t i = 0; i < buff_size; i += num_size)
	{
		// Запись числа
		output << +(*val) << ' ';
		val++;

		// Перенос строки
		NumbersInLineNow++;
		if (NumbersInLineNow >= NumbersInLine)
		{
			output << '\n';
			NumbersInLineNow = 0;
		}
	}

	return 0;
}

// Проверка на Esc
bool CheckForEsc(void)
{
	bool EC = false;
	INPUT_RECORD rec;
	static HANDLE hConInp = GetStdHandle(STD_INPUT_HANDLE);
	DWORD ReadCount;
	while (1)
	{
		PeekConsoleInput(hConInp, &rec, 1, &ReadCount);
		if (ReadCount == 0) break;
		ReadConsoleInput(hConInp, &rec, 1, &ReadCount);
		if (rec.EventType == KEY_EVENT)
			if (rec.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE &&
				rec.Event.KeyEvent.bKeyDown) EC = true;
	}
	return(EC);
}




/*







// Функция ShowMenu выводит меню для выбора формата файла
intptr_t ShowMenu()
{
	FarMenuItem Menu[]
	{
		NULL
	};

	return Info.Menu(
		&MainGuid,				// GUID Плагина
		&MenuGuid,				// GUID Меню
		-1,						// X
		-1,						// Y
		0,						// Максимальная высота
		FMENU_WRAPMODE,			// Флаги
		L"Преобразователь",		// Заголовок
		NULL,					// Нижняя строка меню
		NULL,					// Хелп топик, потом можно добавить
		NULL,					// Ненужные BreakKeys
		NULL,					// и то куда они запишутся
		Menu,					// Само меню
		ARRAYSIZE(Menu)			// И его размер
	);
}


*/
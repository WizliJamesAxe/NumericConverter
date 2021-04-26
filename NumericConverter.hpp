// Библиотека для работы с файлами
#include <fstream>
#include <sstream>

// Библиотека для форматированного вывода чисел
#include <iomanip>

// Библиотека для работы с GUID
#include <initguid.h>
#include "guid.hpp"

// Библиотека для плагинов Far Manager
#include <plugin.hpp>

// Библиотека для создания диалогов
#include <DlgBuilder.hpp>

// Основная информация о плагине
#include "version.hpp"

// Перечислитель для языковых файлов
#include "NumericConverterLng.hpp"

// Получение строки из языкового файла
const wchar_t *GetMsg(int MsgId);

// Отображение сообщения на экране
void ShowMessage(const wchar_t* Mes, FARMESSAGEFLAGS flags = FMSG_NONE | FMSG_MB_OK);

// Отображение Главного диалога программы
bool ShowDialog();

// Функция Конвертации файла с бинарными числами в человекочитаемый файл
errno_t ConvertFile(const wchar_t* FileNameIN, const wchar_t* FileNameOUT);
size_t get_number_size();
template<typename Type>
errno_t buff_to_string(char* buffer, size_t buff_size, std::stringstream& output);

// Обработка нажатия Esc для отмены выполнения плагина
bool CheckForEsc(void);

enum EnumMode
{
  Short,
  Int,
  Double,
  Char,
  Long,
  Float,

  LastMode
};

enum EnumFormat
{
  Signed,
  Unsigned,

  LastFormat
};

// Переменные определяющие формат выходного файла
static int ConvertMode = EnumMode::Int;
static int ConvertFormat = EnumFormat::Signed;

static unsigned int NumbersInLine = 16;
static unsigned int NumbersInLineNow = 0;

static wchar_t suffix[50] = L"_converted.txt";


// Структуры необходимые для работы с Far Manager
static struct PluginStartupInfo Info;
static struct FarStandardFunctions FSF;
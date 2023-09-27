/** ------------------------------------------------------------
 * File          : chsrc.h
 * License       : GPLv3
 * Authors       : Aoran Zeng <ccmywish@qq.com>
 * Created on    : <2023-08-29>
 * Last modified : <2023-09-27>
 *
 * chsrc:
 *
 *   chsrc.c 头文件
 * ------------------------------------------------------------*/

#include "xy.h"
#include "sources.h"


#define App_Prefix "chsrc: "
#define chsrc_success(str) xy_success(xy_2strjoin(App_Prefix, (str)))
#define chsrc_info(str)    xy_info(xy_2strjoin(App_Prefix,  (str)))
#define chsrc_warn(str)    xy_warn(xy_2strjoin(App_Prefix,  (str)))
#define chsrc_error(str)   xy_error(xy_2strjoin(App_Prefix, (str)))


/**
 * 检测二进制程序是否存在
 *
 * @param  check_cmd  检测 `progname` 是否存在的一段命令，一般来说，填 `progname` 本身即可，
 *                    但是某些情况下，需要使用其他命令绕过一些特殊情况，比如 python 这个命令在Windows上
 *                    会自动打开 Microsoft Store，需避免
 *
 * @param  progname   要检测的二进制程序名
 */
bool
does_the_program_exist (char* check_cmd, char* progname)
{
  char* which = check_cmd;

  int ret = system(which);

  // char buf[32] = {0}; sprintf(buf, "错误码: %d", ret);

  if (0!=ret) {
    // xy_warn (xy_strjoin(4, "× 命令 ", progname, " 不存在，", buf));
    xy_warn (xy_strjoin(3, "× 命令 ", progname, " 不存在"));
    return false;
  } else {
    xy_success (xy_strjoin(3, "√ 命令 ", progname, " 存在"));
    return true;
  }
}


/**
 * 用于 _setsrc 函数，检测用户输入的镜像站code，是否存在于该target可用源中
 *
 * @param  target  目标名
 * @param  input   如果用户输入 default 或者 def，则选择第一个源
 */
#define find_mirror(s, input) does_the_input_mirror_exist(s##_sources, s##_sources_n, (char*)#s+3, input)
int
does_the_input_mirror_exist (source_info* sources, size_t size, char* target, char* input)
{
  if (0==size) {
    xy_error(xy_strjoin(3, "chsrc: 当前 ", target, " 无任何可用源，请联系维护者"));
    exit(1);
  }

  if (1==size) {
    xy_success(xy_strjoin(5, "chsrc: ", sources[0].mirror->name, " 是 ", target, " 目前唯一可用镜像站，感谢他们的慷慨支持"));
  }

  if (xy_streql("default", input) || xy_streql("def", input)) {
    xy_info ("chsrc: 默认使用维护团队测速第一的源");
    return 0;
  }

  int idx = 0;
  source_info source = sources[0];

  bool exist = false;
  for (int i=0; i<size; i++)
  {
    source = sources[i];
    if (xy_streql(source.mirror->code, input)) {
      idx = i;
      exist = true;
      break;
    }
  }
  if (!exist) {
    xy_error (xy_strjoin(3, "chsrc: 镜像站 ", input, " 不存在"));
    xy_error (xy_2strjoin("chsrc: 查看可使用源，请使用 chsrc list ", target));
    exit(1);
  }
  return idx;
}


/**
 * 该函数来自 oh-my-mirrorz.py，由我(@ccmywish)翻译为C语言，但功劳和版权属于原作者
 */
char*
to_human_readable_speed (double speed)
{
  char* scale[] = {"Byte/s", "KByte/s", "MByte/s", "GByte/s", "TByte/s"};
  int i = 0;
  while (speed > 1024.0)
  {
    i += 1;
    speed /= 1024.0;
  }
  char* buf = xy_malloc0(64);
  sprintf(buf, "%.2f %s", speed, scale[i]);

  char* new = NULL;
  if (i <= 1 ) new = xy_str_to_red(buf);
  else
  {
    if (i == 2 && speed < 2.00) new = xy_str_to_yellow(buf);
    else new = xy_str_to_green(buf);
  }
  return new;
}


/**
 * 测速代码参考自 https://github.com/mirrorz-org/oh-my-mirrorz/blob/master/oh-my-mirrorz.py
 * 功劳和版权属于原作者，由我(@ccmywish)修改为C语言，并做了额外调整。
 *
 * @return 返回测得的速度，若出错，返回-1
 */
double
test_speed_url (const char* url)
{
  char* time_sec = "6";

/* 现在我们切换至跳转后的链接来测速，不再使用下述判断
  if (xy_str_start_with(url, "https://registry.npmmirror"))
  {
    // 这里 npmmirror 跳转非常慢，需要1~3秒，所以我们给它留够至少8秒测速时间，否则非常不准
    time_sec = "10";
  }
*/

  // 我们用 —L，因为Ruby China源会跳转到其他地方
  // npmmirror 也会跳转
  char* curl_cmd = xy_strjoin(6, "curl -qsL -o ", xy_os_devnull,
                                 " -w \"%{http_code} %{speed_download}\" -m", time_sec ,
                                 " -A chsrc/" Chsrc_Version "  ", url);

  // xy_info (xy_2strjoin("chsrc: 测速 ", url));

  FILE* fp = popen(curl_cmd, "r");
  char buf[64] = {0};
  while(NULL!=fgets(buf, 64, fp));
  // puts(buf);

  // 如果尾部有换行，删除
  char* last_lf = strrchr(buf, '\n');
  if (last_lf) *last_lf = '\0';

  char* split = strchr(buf, ' ');
  if (split) *split = '\0';

  // puts(buf); puts(split+1);
  int http_code = atoi(buf);
  double speed  = atof(split+1);
  char* speedstr = to_human_readable_speed(speed);

  if (200!=http_code) {
    char* httpcodestr = xy_str_to_yellow(xy_2strjoin("HTTP码 ", buf));
    puts (xy_strjoin(3, speedstr, " | ",  httpcodestr));
  } else {
    puts (speedstr);
  }
  return speed;
}


int
dblary_maxidx_(double* array, int size)
{
  double maxval = array[0];
  int maxidx = 0;

  for (int i=1; i<size; i++) {
    if (array[i]>maxval) {
      maxval = array[i];
      maxidx = i;
    }
  }
  return maxidx;
}

/**
 * 自动测速选择镜像站和源
 */
#define auto_select(s) auto_select_(s##_sources, s##_sources_n, (char*)#s+3)
int
auto_select_ (source_info* sources, size_t size, const char* target)
{
  if (0==size) {
    xy_error(xy_strjoin(3, "chsrc: 当前 ", target, " 无任何可用源，请联系维护者"));
    exit(1);
  }

  bool onlyone = false;
  if (1==size) onlyone = true;

  double speeds[size];
  double speed = 0.0;
  for (int i=0;i<size;i++)
  {
    source_info src = sources[i];
    const char* url = src.mirror->__bigfile_url;
    if (NULL==url) {
      chsrc_warn ( xy_strjoin(3, "开发者未提供 ",  src.mirror->code, " 镜像站测速链接，跳过该站点"));
      speed = 0;
    } else {
      printf ("%s",xy_strjoin(3, "chsrc: 测速 ", src.mirror->site , " ... "));
      fflush(stdout);
      speed = test_speed_url (url);
    }
    speeds[i] = speed;
  }
  int fastidx = dblary_maxidx_ (speeds, size);

  if (onlyone)
    xy_success(xy_strjoin(5, "chsrc: ", sources[fastidx].mirror->name, " 是 ", target, " 目前唯一可用镜像站，感谢他们的慷慨支持"));
  else
    xy_success (xy_2strjoin("chsrc: 最快镜像站: ", sources[fastidx].mirror->name));

  return fastidx;
}


#define use_specific_mirror_or_auto_select(input, s) \
  (NULL!=(input)) ? find_mirror(s, input) : auto_select(s)



/**
 * 用于 _setsrc 函数
 */
void
chsrc_say_selection (source_info* source)
{
  xy_info (xy_strjoin(5, "chsrc: 选中镜像站: ", source->mirror->abbr, " (", source->mirror->code, ")"));
}


void
chsrc_say_thanks (source_info* source)
{
  xy_success(xy_2strjoin("chsrc: 感谢镜像提供方: ", source->mirror->name));
}



void
ensure_root ()
{
  char* euid = getenv("$EUID");
  if (NULL==euid) {
    FILE* fp = popen("id -u", "r");
    char buf[10] = {0};
    fgets(buf, 10, fp);
    fclose(fp);
    if (0!=atoi(buf)) goto not_root;
    else return;
  } else {
    if (0!=atoi(euid)) goto not_root;
    else return;
  }
not_root:
  xy_error("chsrc: 请在命令前使用 sudo 来保证必要的权限");
  exit(1);
}


static void
chsrc_run (const char* cmd)
{
  xy_info (xy_2strjoin ("chsrc: 运行 ", cmd));
  system(cmd);
}

static void
chsrc_check_file (const char* path)
{
  char* cmd = NULL;
  if(xy_on_windows) {
    path = xy_unix_path_to_win (path);
    cmd = xy_2strjoin ("type ", path);
  } else {
    cmd = xy_2strjoin ("cat ", path);
  }
  chsrc_run (cmd);
}

static void
chsrc_append_to_file (const char* str, const char* file)
{
  char* cmd = NULL;
  if (xy_on_windows) {
    cmd = xy_strjoin (4, "echo ", str, " >> ", file);
  } else {
    cmd = xy_strjoin (4, "echo '", str, "' >> ", file);
  }
  chsrc_run(cmd);
}

static void
chsrc_overwrite_file (const char* str, const char* file)
{
  char* cmd = NULL;
  if (xy_on_windows) {
    cmd = xy_strjoin (4, "echo ", str, " > ", file);
  } else {
    cmd = xy_strjoin (4, "echo '", str, "' > ", file);
  }
  chsrc_run(cmd);
}

static void
chsrc_backup (const char* path)
{
  char* cmd = xy_strjoin(5, "cp ", path, " ", path, ".bak --backup='t'");
  chsrc_run (cmd);
  chsrc_info ( xy_strjoin (3, "备份文件名 ", path, ".bak"));
}



/* Target Info */
typedef struct {
  void (*setfn)(char* option);
  void (*getfn)(char* option);
  source_info* sources;
  size_t       sources_n;
} target_info;

#define def_target_info(t) target_info t##_target = {t##_setsrc, t##_getsrc, t##_sources, t##_sources_n}

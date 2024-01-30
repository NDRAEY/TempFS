#include "../port.h"

uint32_t str_cdsp2(const char a_str[], char del){
    int x = 0;
    for(size_t i = 0, len = strlen(a_str); i < len; i++){
        if (a_str[i] == del) {
            x++;
        }
    }
    return x;
}


/**
 * @brief ��������� �����
 *
 * @param s1 - ������ 1
 * @param s2 - ������ 2
 *
 * @return ��������� 0 ���� ������ ��������� ��� ������� ����� ����
 */
int my_strcmp(const char *s1, const char *s2) {
    while (*s1 && *s1 == *s2) {
        ++s1;
        ++s2;
    }

    return (*s1 - *s2);
}

/**
 * @brief ��������� �����
 *
 * @param str1 - ������ 1
 * @param str2 - ������ 2
 *
 * @return bool - ��������� true ���� ������ ���������
 */
bool strcmpn(const char *str1, const char *str2){
    return my_strcmp(str1, str2) == 0;
}

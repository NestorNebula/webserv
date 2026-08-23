/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   libft.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kdonlon <kdonlon@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/20 11:41:52 by kdonlon           #+#    #+#             */
/*   Updated: 2026/08/23 10:54:01 by kdonlon          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.hpp"

void	*ft_memset(void *dst, int val, size_t cnt)
{
    if (dst == NULL)
        return (NULL);

    unsigned char	*d = (unsigned char*) dst;
    unsigned char	*e = d + cnt;
    
    while (d < e)
        *d++ = val;

    return (dst);
}
void	*ft_memcpy(void *dst, const void *src, size_t cnt)
{
	size_t			i;
	unsigned char	*d;
	unsigned char	*s;

	if (!dst && !src)
		return (NULL);
	d = (unsigned char *) dst;
	s = (unsigned char *) src;
	i = 0;
	while (i < cnt)
	{
		d[i] = s[i];
		i++;
	}
	return (dst);
}
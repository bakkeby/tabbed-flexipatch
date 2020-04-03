void
showbar(const Arg *arg)
{
	barvisibility = arg->i;
	drawbar();
}
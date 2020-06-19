import click
import requests
from collections import OrderedDict
import json
import copy

CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])

@click.group(context_settings=CONTEXT_SETTINGS)
@click.argument('c3po_url')
@click.pass_context
def c3pocli(ctx, c3po_url):
    """This script connects to the c3po endpoint C3PO_URL and issues rest commands against it"""
    ctx.obj = {}
    ctx.obj['C3PO_URL'] = c3po_url
    pass

@click.group()
@click.pass_context
def admin(ctx):
    pass

@click.group()
@click.pass_context
def stats(ctx):
    pass

@click.group()
@click.pass_context
def logger(ctx):
    pass


@click.command()
@click.pass_context
def describe_stats_frequency(ctx):
    url = ctx.obj['C3PO_URL'] + "/statfreq"
    r = requests.get(url)
    click.echo(r.json())

@click.command()
@click.pass_context
@click.option('--freq', '-f', required=True, type=int, help='Stats generation interval in millisecond')
def set_stats_frequency(ctx, freq):
    url = ctx.obj['C3PO_URL'] + "/statfreq"
    r = requests.post(url, json={"statfreq": freq})
    click.echo(r.json())

@click.command()
@click.pass_context
def describe_stats_live(ctx):
    url = ctx.obj['C3PO_URL'] + "/statlive"
    r = requests.get(url)
    res = r.json()
    new_res = copy.deepcopy(res)
    click.echo(json.dumps(res))

@click.command()
@click.pass_context
def describe_stats_all(ctx):
    url = ctx.obj['C3PO_URL'] + "/statliveall"
    r = requests.get(url)
    res = r.json()
    new_res = copy.deepcopy(res)
    click.echo(json.dumps(res))

@click.command()
@click.pass_context
def describe_loggers(ctx):
    url = ctx.obj['C3PO_URL'] + "/logger"
    r = requests.get(url)
    click.echo(r.json())

@click.command()
@click.pass_context
def describe_stats_logging(ctx):
    url = ctx.obj['C3PO_URL'] + "/statlogging"
    r = requests.get(url)
    click.echo(r.json())

@click.command()
@click.pass_context
@click.option('--name', '-n', required=True, help='Enter stat logging name suppress or all')
def set_stats_logging(ctx, name):
    url = ctx.obj['C3PO_URL'] + "/statlogging"
    r = requests.post(url, json={"statlog": name })
    click.echo(r.json())

@click.command()
@click.pass_context
@click.option('--name', '-n', required=True, help='Logger name')
@click.option('--level', '-l', required=True, type=int, help='Logger level')
def set_logger_level(ctx, name, level):
    url = ctx.obj['C3PO_URL'] + "/logger"
    r = requests.post(url, json={"name": name, "level": level})
    click.echo(r.json())


@click.command()
@click.pass_context
def describe_oss_options(ctx):
    url = ctx.obj['C3PO_URL'] + "/ossoptions"
    r = requests.get(url)
    click.echo(r.json())

@click.command()
@click.pass_context
def dnscache_refresh(ctx):
    url = ctx.obj['C3PO_URL'] + "/dnscache_refresh"
    r = requests.post(url, json={})
    click.echo(r.json())

c3pocli.add_command(admin)
c3pocli.add_command(stats)
c3pocli.add_command(logger)

admin.add_command(describe_oss_options)
admin.add_command(dnscache_refresh)

stats.add_command(describe_stats_frequency)
stats.add_command(set_stats_frequency)
stats.add_command(describe_stats_live)
stats.add_command(describe_stats_all)
stats.add_command(describe_stats_logging)
stats.add_command(set_stats_logging)

logger.add_command(describe_loggers)
logger.add_command(set_logger_level)


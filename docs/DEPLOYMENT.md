# PopsTracker Deployment Guide

Complete deployment guide for hosting PopsTracker in production.

## Deployment Options

| Option | Cost | Difficulty | Best For |
|--------|------|------------|----------|
| Railway | Free tier available | Easy | Quick start |
| Render | Free tier available | Easy | Quick start |
| DigitalOcean | $5/month | Medium | Full control |
| AWS | Variable | Hard | Enterprise |
| Docker (VPS) | VPS cost | Medium | Self-hosted |

---

## Option 1: Railway (Recommended for Beginners)

Railway offers a free tier and auto-deploys from GitHub.

### Steps

1. **Sign up** at [railway.app](https://railway.app)

2. **Create New Project**
   - Click "New Project"
   - Select "Deploy from GitHub repo"
   - Authorize and select your repository

3. **Add MongoDB**
   - Click "New" in your project
   - Select "Database" → "MongoDB"
   - Railway auto-configures the connection

4. **Configure Backend Service**
   - Click on your backend service
   - Go to "Variables"
   - Add:
     ```
     NODE_ENV=production
     ```
   - The `MONGODB_URI` is auto-configured

5. **Set Root Directory**
   - Go to Settings → Root Directory
   - Set to `backend`

6. **Get Domain**
   - Go to Settings → Domains
   - Click "Generate Domain"
   - Your API is now live!

7. **Update Firmware**
   - Edit `firmware/src/config.h`
   - Set `SERVER_HOST` to your Railway domain

### Cost
- Free tier: 500 hours/month, 5GB storage
- Hobby plan: $5/month for always-on

---

## Option 2: Docker on VPS

### Prerequisites
- VPS with Docker installed (DigitalOcean, Linode, Vultr, etc.)
- Domain name (optional but recommended)

### Steps

1. **SSH into your server**
   ```bash
   ssh root@your-server-ip
   ```

2. **Install Docker and Docker Compose**
   ```bash
   curl -fsSL https://get.docker.com | bash
   apt-get install docker-compose-plugin
   ```

3. **Clone the repository**
   ```bash
   git clone https://github.com/your-repo/PopsTrackerNew.git
   cd PopsTrackerNew
   ```

4. **Configure environment**
   ```bash
   cp backend/.env.example backend/.env
   nano backend/.env
   ```

5. **Start services**
   ```bash
   docker compose up -d
   ```

6. **Check status**
   ```bash
   docker compose ps
   docker compose logs -f backend
   ```

### With SSL (Recommended)

1. **Install Caddy** (easier than nginx for SSL)
   ```bash
   apt-get install -y debian-keyring debian-archive-keyring apt-transport-https
   curl -1sLf 'https://dl.cloudsmith.io/public/caddy/stable/gpg.key' | gpg --dearmor -o /usr/share/keyrings/caddy-stable-archive-keyring.gpg
   curl -1sLf 'https://dl.cloudsmith.io/public/caddy/stable/debian.deb.txt' | tee /etc/apt/sources.list.d/caddy-stable.list
   apt-get update
   apt-get install caddy
   ```

2. **Configure Caddy**
   ```bash
   nano /etc/caddy/Caddyfile
   ```

   ```
   your-domain.com {
       reverse_proxy localhost:3000
   }

   dashboard.your-domain.com {
       reverse_proxy localhost:8080
   }
   ```

3. **Restart Caddy**
   ```bash
   systemctl reload caddy
   ```

---

## Option 3: DigitalOcean App Platform

1. **Create App**
   - Go to [cloud.digitalocean.com](https://cloud.digitalocean.com)
   - Click "Create" → "Apps"
   - Connect GitHub repo

2. **Configure Services**
   - Backend: Set source directory to `backend`
   - Add MongoDB from DigitalOcean marketplace

3. **Deploy**
   - Click "Deploy"
   - Get your app URL

---

## Database Setup

### MongoDB Atlas (Cloud - Recommended)

1. **Create account** at [mongodb.com/atlas](https://www.mongodb.com/atlas)

2. **Create free cluster**
   - M0 Sandbox (free forever)
   - Choose region close to your server

3. **Configure access**
   - Database Access: Create user with password
   - Network Access: Add your server IP (or 0.0.0.0/0 for any)

4. **Get connection string**
   - Click "Connect" → "Connect your application"
   - Copy the connection string
   - Replace `<password>` with your password

5. **Update .env**
   ```
   MONGODB_URI=mongodb+srv://username:password@cluster.mongodb.net/popstracker
   ```

---

## Environment Variables

### Required

| Variable | Description | Example |
|----------|-------------|---------|
| `PORT` | Server port | `3000` |
| `MONGODB_URI` | MongoDB connection | `mongodb://localhost:27017/popstracker` |
| `NODE_ENV` | Environment | `production` |

### Optional

| Variable | Description |
|----------|-------------|
| `FRONTEND_URL` | CORS allowed origin |
| `VAPID_*` | Web push keys |
| `SMTP_*` | Email settings |
| `TWILIO_*` | SMS settings |

---

## SSL Certificate

### Option A: Caddy (Automatic)
Caddy automatically obtains and renews SSL certificates.

### Option B: Certbot (for Nginx)
```bash
apt-get install certbot python3-certbot-nginx
certbot --nginx -d your-domain.com
```

### Option C: Cloudflare (Free)
1. Add domain to Cloudflare
2. Point DNS to your server
3. Enable "Full (Strict)" SSL mode

---

## Monitoring

### PM2 (Process Manager)
```bash
npm install -g pm2
pm2 start backend/src/index.js --name popstracker
pm2 monit
pm2 logs
```

### Health Check
```bash
# Cron job to check health
*/5 * * * * curl -f http://localhost:3000/health || systemctl restart popstracker
```

### Logs
```bash
# Docker
docker compose logs -f backend

# PM2
pm2 logs popstracker

# System
journalctl -u popstracker -f
```

---

## Backup

### MongoDB Backup
```bash
# Create backup
mongodump --uri="mongodb://localhost:27017/popstracker" --out=/backup/$(date +%Y%m%d)

# Restore
mongorestore --uri="mongodb://localhost:27017/popstracker" /backup/20240101
```

### Automated Backups
```bash
# Add to crontab
0 2 * * * mongodump --uri="mongodb://localhost:27017/popstracker" --out=/backup/$(date +%Y%m%d) && find /backup -mtime +7 -delete
```

---

## Updating

### Git Deploy
```bash
cd /path/to/PopsTrackerNew
git pull origin main
docker compose down
docker compose up -d --build
```

### Zero Downtime
```bash
docker compose up -d --build --no-deps backend
```

---

## Security Checklist

- [ ] Use HTTPS (SSL certificate)
- [ ] Set strong MongoDB password
- [ ] Restrict MongoDB network access
- [ ] Use environment variables (never commit secrets)
- [ ] Enable firewall (only allow 80, 443, 22)
- [ ] Regular security updates
- [ ] Backup database regularly
- [ ] Monitor for unusual activity

---

## Cost Estimate

### Budget Option (~$5-10/month)
- DigitalOcean Basic Droplet: $6/month
- MongoDB Atlas Free Tier: $0
- Cloudflare Free: $0
- **Total: ~$6/month**

### Recommended Option (~$15-20/month)
- DigitalOcean Droplet (2GB): $12/month
- MongoDB Atlas M2: $9/month
- Cloudflare Free: $0
- **Total: ~$21/month**

### Enterprise Option
- AWS/GCP managed services
- Multi-region deployment
- 24/7 monitoring
- **Variable pricing**

---

## Troubleshooting

### Backend not starting
```bash
# Check logs
docker compose logs backend

# Check MongoDB connection
docker compose exec backend node -e "require('mongoose').connect(process.env.MONGODB_URI).then(() => console.log('OK'))"
```

### No data from device
1. Check device serial output for errors
2. Verify server URL in firmware config
3. Check network connectivity
4. Verify APN settings

### High latency
1. Check server location vs device location
2. Optimize MongoDB queries
3. Add database indexes
4. Consider CDN for static assets

---

## Support

- GitHub Issues: Report bugs and feature requests
- Documentation: Check SETUP.md and WIRING.md
- Community: Join our Discord server
